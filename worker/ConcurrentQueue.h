#pragma once

typedef unsigned int u32;

template<class T>
class ConcurrentQueue
{
private:
	struct Node;
	struct Pointer
	{
		Node* ptr1;
		volatile u32 cnt;
		bool operator==(Pointer& p2)
		{
			return this->ptr1==p2.ptr1 && this->cnt==p2.cnt;
		}
		Pointer& operator=(volatile Pointer& p2)
		{
			this->ptr1=p2.ptr1;
			this->cnt=p2.cnt;
			return *this;
		}
	};
	struct Node
	{
		Pointer next;
		T data;
	};

	Pointer head_;
	Pointer tail_;

	static inline bool CompareAndExchange(volatile Pointer* toXch,volatile Node* ptr11,
		u32 cnt1,volatile Node* ptr12,u32 cnt2)
	{
		__asm
		{
			push ebx
			push esi
			mov ebx,ptr12
			mov ecx,cnt2
			mov eax,ptr11
			mov edx,cnt1
			mov esi,toXch
			lock cmpxchg8b [esi]
			sete al
			pop esi
			pop ebx
		}
	}

public:
	ConcurrentQueue()
	{
		Node* n=new Node();
		n->next.ptr1=NULL;
		head_.ptr1=tail_.ptr1=n;
		head_.cnt=tail_.cnt=n->next.cnt=0;
	}

	void Enqueue(T& data)
	{
		Node* node=new Node();
		node->data=data;
		node->next.ptr1=NULL;

		Pointer t,n;
		while(true)
		{
			t=tail_;
			n=t.ptr1->next;
			if(t==tail_)
			{
				if(n.ptr1==NULL)
				{
					if(CompareAndExchange(&t.ptr1->next,n.ptr1,n.cnt,node,n.cnt+1))
						break;
				}
				else
				{
					CompareAndExchange(&tail_,t.ptr1,t.cnt,n.ptr1,t.cnt+1);
				}
			}
		}
		CompareAndExchange(&tail_,t.ptr1,t.cnt,node,t.cnt+1);
	}

	bool Dequeue(T* data)
	{
		Pointer h,t,n;
		while(true)
		{
			h=head_;
			t=tail_;
			n=h.ptr1->next;
			if(h==head_)
			{
				if(h.ptr1==t.ptr1)
				{
					if(n.ptr1==NULL)
						return false;
					CompareAndExchange(&tail_,t.ptr1,t.cnt,n.ptr1,t.cnt+1);
				}
				else
				{
					*data=n.ptr1->data;
					if(CompareAndExchange(&head_,h.ptr1,h.cnt,n.ptr1,h.cnt+1))
						break;
				}
			}
		}
		delete h.ptr1;
		return true;
	}
};

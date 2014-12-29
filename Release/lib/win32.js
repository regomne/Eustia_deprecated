/// @file
/// @brief windows API支持模块

/// @cond
var asm=require('asm');
var native=require('native');
requireAll('quickfunc',global);
require('mystring')(global);
/// @endcond

/// 为Win32命名空间增加一个API。
/// @brief 为Win32命名空间增加一个API
/// @param name 要获取的api模块名和导出名形如ntdll.ZwGetContextThread。其中ntdll,kernel32,user32,gdi32中的函数可以不写模块名
/// @function Win32.add(name)

/// 调用堆内存分配函数在进程堆内分配内存、
/// @brief 调用堆内存分配函数在进程堆内分配内存
/// @param size 内存大小
/// @return 内存指针，分配失败抛出异常
/// @function Win32.newMem(size)

/// 释放进程堆内存上分配的内存。
/// @brief 释放进程堆内存上分配的内存
/// @param ptr 内存指针
/// @function Win32.deleteMem(ptr)

/// 在dbgView上输出信息。
/// @brief 在dbgView上输出信息
/// @param fmt 格式化字符串
/// @param ... 根据格式化字符串需要的其他参数
/// @function Win32.dbgOut(fmt)

/// 根据结构定义解析一个结构地址指针。结构定义形如：\n
/// {\n
/// ExceptionCode:{type:'I',offset:0},\n
/// ExceptionFlags:{type:'I',offset:4},\n
/// ExceptionRecord:{type:'I',offset:8},\n
/// ExceptionAddress:{type:'I',offset:0xc},\n
/// NumberParameters:{type:'I',offset:0x10},\n
/// };
/// @brief 根据结构定义解析一个结构地址指针
/// @param defs 结构定义
/// @param addr 待解析的结构地址
/// @return 返回结构对象指针，可以直接以属性名引用其中的字段
/// @function Win32.struct(defs,addr)

var Win32={
	_apis:[
		'kernel32.HeapAlloc',
		'kernel32.HeapFree',
		'kernel32.GetProcessHeap',
		'kernel32.VirtualProtect',
		'kernel32.LoadLibraryA',
    'kernel32.GetModuleHandleA',
    'kernel32.GetModuleHandleW',
    'kernel32.GetProcAddress',
		'kernel32.OpenProcess',
		'winmm.timeGetTime',
		'kernel32.OutputDebugStringA',
		'kernel32.OutputDebugStringW',
		'user32.MessageBoxA',
		'user32.MessageBoxW',
	],
	
	init:function()
	{
		that=this;
		this._apis.forEach(function(name)
		{
			that.add(name);
		});
	},
	
	add:function(name)
	{
		var newName=name;
		if(name.indexOf('.')!=-1)
			newName=name.split('.')[1];
		if(name[0]=='#')
		{
			var addr=native.getAPIAddress(name.slice(1));
			if(!addr)
				throw new Error("Can't find api");
			this[newName]=asm.makeCdeclFunction(addr);
		}
		else
		{
			var addr=native.getAPIAddress(name);
			if(!addr)
				throw new Error("Can't find api");
			this[newName]=asm.makeStdcallFunction(addr);
		}
	}
};

Win32.init();

Win32.newMem=function(size)
{
	var ret=Win32.HeapAlloc(Win32.GetProcessHeap(),0,size);
	if(ret==0)
		throw "alloc meme error!";
	return ret;
}

Win32.deleteMem=function(ptr)
{
	Win32.HeapFree(Win32.GetProcessHeap(),0,ptr);
}

Win32.dbgOut=function(format){
	var args=[];
	for(var i=1;i<arguments.length;i++)
		args.push(arguments[i]);
	var s=(format.format.apply(format,[args])+'\0').encode();
	Win32.OutputDebugStringW(s);
};

Win32.struct=function(defs,addr)
{
	for(var name in defs)
	{
		var def=defs[name];
		var rfunc=null;
		var wfunc=null;
		var daddr=addr+def.offset;

		switch(def.type)
		{
		case 'I':
			rfunc=u32;
			wfunc=wu32;
			break;
		case 'H':
			rfunc=u16;
			wfunc=wu16;
			break;
		case 'B':
			rfunc=u8;
			wfunc=wu8;
			break;
		default:
			var subdefs=Win32[def.type];
			if(subdefs!=undefined)
			{

				rfunc=function(addr){return new Win32.struct(subdefs,addr)};
				//wfunc
			}
			else
			{
				throw new Error('unk type: '+def.type);
			}
		}
		// Object.defineProperty(this,name,{
		// 	get:function()
		// 	{
		// 		return rfunc(daddr);
		// 	},
		// 	set:function(val)
		// 	{
		// 		wfunc(daddr,val);
		// 	}
		// });
		this[name]=rfunc(daddr);
	}
}

Win32.EXCEPTION_RECORD ={
	ExceptionCode:{type:'I',offset:0},
	ExceptionFlags:{type:'I',offset:4},
	ExceptionRecord:{type:'I',offset:8},
	ExceptionAddress:{type:'I',offset:0xc},
	NumberParameters:{type:'I',offset:0x10},
};

Win32.CONTEXT={
	ContextFlags:{type:'I',offset:0},
	Dr0:{type:'I',offset:4},
	Dr1:{type:'I',offset:8},
	Dr2:{type:'I',offset:12},
	Dr3:{type:'I',offset:16},
	Dr6:{type:'I',offset:20},
	Dr7:{type:'I',offset:24},
	//...
	Edi:{type:'I',offset:156},
	Esi:{type:'I',offset:160},
	Ebx:{type:'I',offset:164},
	Edx:{type:'I',offset:168},
	Ecx:{type:'I',offset:172},
	Eax:{type:'I',offset:176},
	Ebp:{type:'I',offset:180},
	Eip:{type:'I',offset:184},
	SegCs:{type:'I',offset:188},
	EFlags:{type:'I',offset:192},
	Esp:{type:'I',offset:196},
};

module.exports=Win32;
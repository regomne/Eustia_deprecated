
function callFunction(addr,callType,regs) //其他参数附在后面
{
	var rcallType=0;
	switch(callType)
	{
		case 'cdecl':
			rcallType=0;
			break;
		case 'stdcall':
			rcallType=1;
			break;
		default:
			print(callType);
			throw "call type error!";
	}
	
	var rregs={};
	for(var reg in regs)
	{
		switch(reg)
		{
			case 'eax': rregs[7]=regs[reg]; break;
			case 'ecx': rregs[6]=regs[reg]; break;
			case 'edx': rregs[5]=regs[reg]; break;
			case 'ebx': rregs[4]=regs[reg]; break;
//			case 'esp': rregs[3]=regs[reg]; break;
//			case 'ebp': rregs[2]=regs[reg]; break;
			case 'esi': rregs[1]=regs[reg]; break;
			case 'edi': rregs[0]=regs[reg]; break;
		}
	}
	
	var rargs=[];
	for(var i=arguments.length-1;i>=3;i--)
	{
		rargs.push(arguments[i]);
	}
	
	return _CallFunction(addr,rcallType,rregs,rargs);
}

function setProperty(obj,key,name,prop)
{
	var rProp=0;
	if(prop!=undefined)
	{
		prop=prop.toString();
		if(prop.search('r')!=-1) rProp|=1; //read only
		if(prop.search('e')!=-1) rProp|=(1<<1); //don't enum
		if(prop.search('d')!=-1) rProp|=(1<<2); //don't delete
	}
	_SetProperty(obj,key,name,rProp);
}

function mread(addr,size)
{
	return _Mread(addr,size);
}

function dumpMemory()
{
	return _DumpMemory.apply(this,arguments);
}

function suspendAllThreads(processId,ignoreList)
{
	var getpid=makeSimpleFunction(getAPIAddress('GetCurrentProcessId'),'stdcall');
	if(ignoreList==undefined) ignoreList=[];
	if(processId==undefined) processId=getpid();
	
	ignoreList.push(_MyWindowThreadId);
	return _SuspendAllThreads(processId,ignoreList);
}

function resumeAllThreads(idListPtr)
{
	return _ResumeAllThreads(idListPtr);
}

function getAPIAddress()
{
	return _GetAPIAddress.apply(this,arguments);
}

function getMemoryBlocks(hp)
{
	if(hp==undefined)
		hp==-1;
	return _GetMemoryBlocks(hp);
}
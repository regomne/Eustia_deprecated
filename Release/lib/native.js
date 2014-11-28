
var exp={};
exp.callFunction=function(addr,callType,regs) //其他参数附在后面
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
			throw new Error("call type error!");
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

// exp.setProperty=function(obj,key,name,prop)
// {
// 	var rProp=0;
// 	if(prop!=undefined)
// 	{
// 		prop=prop.toString();
// 		if(prop.search('r')!=-1) rProp|=1; //read only
// 		if(prop.search('e')!=-1) rProp|=(1<<1); //don't enum
// 		if(prop.search('d')!=-1) rProp|=(1<<2); //don't delete
// 	}
// 	_SetProperty(obj,key,name,rProp);
// }

exp.disassemble=function (addr)
{
	return _Disassemble(addr);
}

exp.mread=function (addr,size)
{
	try
	{
		return _Mread(addr,size);
	}
	catch(e)
	{
		throw new Error(e);
	}
}

exp.mwrite=function (addr,buff,start,len)
{
	if(buff==undefined)
		throw new Error("arg2 must be a string");
	if(start==undefined)
		start=0;
	if(len==undefined)
		len=buff.length-start;
	return _Mwrite(addr,buff,start,len);
}

exp.readText=function (fname)
{
	return _ReadText(lookupFile(fname));
}

exp.dumpMemory=function ()
{
	return _DumpMemory.apply(this,arguments);
}

exp.suspendAllThreads=function (processId,ignoreList)
{
	var getpid=makeSimpleFunction(getAPIAddress('GetCurrentProcessId'),'stdcall');
	if(ignoreList==undefined) ignoreList=[];
	if(processId==undefined) processId=getpid();
	
	ignoreList.push(_MyWindowThreadId);
	return _SuspendAllThreads(processId,ignoreList);
}

exp.resumeAllThreads=function (idListPtr)
{
	return _ResumeAllThreads(idListPtr);
}

exp.getAPIAddress=function ()
{
	return _GetAPIAddress.apply(this,arguments);
}

exp.getMemoryBlocks=function (hp)
{
	if(hp==undefined)
		hp=-1;
	return _GetMemoryBlocks(hp);
}

exp.newCallback=function (funcId,argCnt,callType)
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
	
	return _NewCallback(funcId,argCnt,rcallType);
}

exp.deleteMem=function (addr)
{
	return _DeleteMem(addr);
}

exp.writeText=function (fname,buff,isUnicode)
{
	if(isUnicode==undefined)
		isUnicode=0;
	
	if(fname[0]!='\\' && fname[1]!=':')
		fname=_DllPath+fname;
	return _WriteText(fname,buff,isUnicode);
}

exp.existsFile=function (fname)
{
	return _ExistsFile(fname);
}

exp.checkInfoHook=function(addr)
{
	return _CheckInfoHook(addr);
}

exp.unhook=function(addr)
{
	return _Unhook(addr);
}

exp.toFloat=function(val)
{
	return _ToFloat(val);
}

exp.toFloatp=function(p)
{
	return _ToFloatp(p);
}

exp.toDoublep=function(p)
{
	return _ToDoublep(p);
}

module.exports=exp;
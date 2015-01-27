/// @file
/// @brief C++导出的函数的封装模块

var exp={};

/// 调用指定地址的native函数，可以指定调用约定类型以及调用时的寄存器值。\n
/// 寄存器的值将会被转换为整数，其他参数中，number即bool转换为整型，string被转换为char*，array被转换为数组（void*）。传入其他类型则强制转换为整数。
/// @brief 调用指定地址的函数
/// @param addr 函数地址
/// @param callType 调用约定类型，可以是stdcall cdecl中的一个
/// @param regs 寄存器对象
/// @param ... 之后跟随所有需要push的参数
/// @return 调用之后的eax值
/// @function callFunction(addr,callType,regs)

/// 反汇编指定地址的指令，返回指令字符串和指令长度。
/// @brief 反汇编指定地址的指令
/// @param addr 内存地址
/// @return 对象，形如{string:'push eax',length:1}
/// @remark 反汇编失败会抛出异常
/// @function disassemble(addr)

/// 读取指定地址的内存，以字符串形式返回。
/// @brief 读取指定地址的内存
/// @param addr 内存地址
/// @param size 长度
/// @return 读取到的内容，字符串形式，每个字节占一个字符。
/// @function mread(addr,size)

/// 在指定的内存地址写入指定内容。
/// @brief 在指定的内存地址写入指定内容
/// @param addr 内存地址
/// @param buff 待写入的内容，字符串形式，取每个字符的低8位
/// @param start 字符串的起始偏移，默认为0
/// @param len 要写入的长度，默认为从start到字符串末尾
/// @return 实际写入的字节数
/// @function mwrite(addr,buff,start,len)

/// 以纯文本的方式读取一个文件，返回字符串。读取时判断UTF-8和UTF-16LE的BOM，如果两者都不是的话，则以utf8无bom编码读入。
/// @brief 以纯文本方式读取指定文件
/// @param fname 文件名
/// @return 读取到的文本
/// @remark 读取时会依次搜索__Path，若都没有找到对应文件，则抛出异常。
/// @function readText(fname)

/// dump目标进程的一块内存到文件中。
/// @brief dump目标进程的一块内存到文件中
/// @param handle 进程句柄，-1代表本进程
/// @param addr 内存地址
/// @param size 内存长度
/// @param fname 要保存的文件名，需要全路径
/// @return 若成功，返回true，否则false
/// @function dumpMemory(handle,addr,size,fname)

/// 暂停目标进程的所有线程，v8当前执行的线程以及忽略列表中的线程除外。
/// @brief 暂停目标进程的所有线程
/// @param processId 目标进程id
/// @param ignoreList 要忽略的线程id列表，array类型
/// @return 返回被暂停的进程列表指针，必须保存以备恢复时传入
/// @function suspendAllThreads(processId,ignoreList)

/// 恢复全部线程（只限本进程）
/// @brief 恢复全部线程
/// @param idListPtr suspendAllThreads返回的列表指针
/// @function resumeAllThreads(idListPtr)

/// 获取api地址。
/// @brief 获取api地址
/// @param api 要获取的api模块名和导出名形如ntdll.ZwGetContextThread。其中ntdll,kernel32,user32,gdi32中的函数可以不写模块名
/// @return 获得api地址，若失败则返回0
/// @function getAPIAddress(api)

/// 获取当前进程内存块快照，包括本进程所有内存块的基址等信息。
/// @brief 获取当前进程内存块快照
/// @param hp 进程句柄，-1为本进程，默认-1
/// @return 返回进程内存块信息数组，数组对象包括baseAddress,allocationBase,regionSize,protect,allocationProtect 5个属性
/// @function getMemoryBlocks(hp)

/// 新建一个native callback。
/// @brief 新建一个native callback
/// @param funcId 函数id，用以在回调时区分，每个回调须不同
/// @param argCnt 参数个数
/// @param callType 调用类型
/// @return 返回创建的函数指针
/// @function newCallback(funcId,argCnt,callType)

/// 使用C++操作符delete char*清除内存。
/// @brief 释放内存
/// @param addr 内存地址
/// @function deleteMem(addr)

/// 把纯文本写入文件。
/// @brief 把纯文本写入文件
/// @param fname 文件名，如果是相对路径，则以eustia路径为基础
/// @param buff 待写入的字符串
/// @param isUnicode 如果为真则使用unicode编码写入，否则只写入每个字符的低8位
/// @function writeText(fname,buff,isUnicode)

/// 判断指定的文件名是否是一个存在的文件。
/// @brief 判断指定文件是否存在
/// @param fname 文件名
/// @return 若存在指定文件则为true，否则false
/// @remark 只检查是否为文件，若为目录依然返回false
/// @function existsFile(fname)

/// 将指定的uint转换成float。
/// @brief 将指定的uint转换成float
/// @param val 无符号整数
/// @return 转换后的float
/// @function toFloat(val)

/// 将指定地址的uint转换成float。
/// @brief 将指定地址的uint转换成float
/// @param p 无符号整数指针
/// @return 转换后的float
/// @function toFloatp(p)

/// 将指定地址的longlong转换成double。
/// @brief 将指定地址的longlong转换成double
/// @param p 无符号长整数指针
/// @return 转换后的double
/// @function toDoublep(p)

/// 创建一个带NamedAccessor的函数。
/// 可对返回的函数使用var obj=new Func()来获得一个使用该访问器的对象。
/// @brief 创建一个带NamedAccessor的函数
/// @param getter 访问器
/// @return 返回函数
/// @function newFunctionWithNamedAccessor(getter)

/// @cond
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
			//case 'esp': rregs[3]=regs[reg]; break;
			//case 'ebp': rregs[2]=regs[reg]; break;
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
		throw e;
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

exp.newFunctionWithNamedAccessor=function(getter)
{
	return _NewFunctionWithNamedAccessor(getter);
}

module.exports=exp;

/// @endcond
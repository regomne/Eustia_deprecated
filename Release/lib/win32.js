/// @file
/// @brief windows API支持模块

/// @cond
var asm=require('asm');
var native=require('native');
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

var Win32={
	_apis:[
		'kernel32.HeapAlloc',
		'kernel32.HeapFree',
		'kernel32.GetProcessHeap',
		'kernel32.VirtualProtect',
		'kernel32.LoadLibraryA',
		'kernel32.OpenProcess',
		'winmm.timeGetTime',
		'kernel32.OutputDebugStringA',
		'kernel32.OutputDebugStringW',
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

module.exports=Win32;
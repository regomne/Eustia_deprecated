/// @file asm.js
/// @brief 与汇编、hook等相关的模块

/// @cond
var native=require('native');
var utils=require('utils');
requireAll('quickfunc',global);
requireAll('func',global);
require('mystring')(String);
/// @endcond

/// 反汇编一行指令，并输出
/// @brief 反汇编一行指令
/// @param addr 待反汇编的地址
function printOneDisasm(addr)
{
	var dis=native.disassemble(addr);
	var opCodes='';
	var bytes=native.mread(addr,dis.length);
	for(var i=0;i<dis.length;i++)
	{
		opCodes+=bytes.charCodeAt(i).toString(16).rjust(2,'0')+' ';
	}
	print(addr.toString(16).rjust(8,'0')+'  '+opCodes.ljust(24)+dis.string);
	return dis.length;
}

/// 反汇编指定字节数的指令，并输出
/// @brief 反汇编指定长度的指令
/// @param addr 待反汇编的地址
/// @param cnt 待反汇编的字节长度（最长会超出15字节）
function printDisasms(addr,cnt)
{
	var cur=addr;
	while(cur<addr+cnt)
	{
		cur+=printOneDisasm(cur);
	}
	return cur-addr;
}

/// 生成显示信息用的函数。\n
/// 用于生成的指令由以下形式构成\n
/// {exp1}[\@cond1]\#{exp2}[\@cond2]\#...\n
/// 其中exp是用于print的表达式，\@之后是可选的条件。每两个指令之间使用“\#”隔开。\n
/// 比如 @b eax\@ecx==0\#u32(esp+4) 即为“在ecx为0的时候打印eax的值，并且总是打印dword ptr [esp+4]的值”。
/// @brief 根据指令生成显示信息的函数
/// @param exp 用于生成的指令表达式
/// @return 生成的函数。
function makeHookerFuncFromExp(exp)
{
	var eles=exp.split('#').map(function(e){return e.split('@')});


	return function(regs){
		with(regs)
		{
			eles.forEach(function(ele){
				if(ele[1])
				{
					if(eval(ele[1]))
						print(eval(ele[0]));
				}
				else
					print(eval(ele[0]));
			});
		}
	};
}

/// 生成统计信息用的函数，表达式格式与makeHookerFuncFromExp中的相同
/// @brief 生成统计信息用的函数
/// @param exp 用于进行统计的表达式
/// @param rslt 存储统计结果的对象
/// @return 返回统计函数
function makeSummFuncFromExp(exp, rslt)
{
	var eles=exp.split('#').map(function(e){return e.split('@')});

	return function(regs){
		with(regs)
		{
			for(var i=0;i<eles.length;i++)
			{
				var ele=eles[i];
				var val;
				if(ele[1])
				{
					if(eval(ele[1]))
						val=eval(ele[0]).toString();
					else
						return;
				}
				else
					val=eval(ele[0]).toString();

				if(!rslt[i])
					rslt[i]={};
				if(!rslt[i][val])
					rslt[i][val]=1;
				else
					rslt[i][val]++;
			}
		}
	};
}

/// hook指定地址，在执行到该地址时，转入指定的js函数，执行完毕之后返回原处继续执行，视返回值决定是否修改寄存器的值。
/// @brief hook一个地址
/// @param addr 待hook的地址
/// @param func hook之后进入的函数
/// @param tag 本hook的名字
/// @return hook成功的话，返回此hook的全局唯一id，否则返回false
/// @function Hooker.checkInfo(addr,func,tag)

/// hook函数指定的回调函数，用在Hooker.checkInfo内。该函数接受一个寄存器结构，若函数返回true，则寄存器结构内的所有寄存器将会在返回前应用到真实寄存器中，否则忽略。
/// @brief Hooker.checkInfo中使用的回调
/// @param regs 寄存器对象，包括eax ecx edx ebx esp ebp esi edi eflags
/// @return 是否设置寄存器值
/// @function hookerProc(regs)

/// unhook指定地址。
/// @brief unhook指定地址
/// @param addr 需要unhook的地址
/// @function Hooker.unHook(addr)

/// 通过id进行unhook。
/// @brief 通过id进行unhook
/// @param id 指定的id
/// @function Hooker.unHookById(id)

/// unhook所有hook点。
/// @brief unhook所有hook点
/// @function Hooker.unHookAll()

/// 获取所有hook点列表，列表中每个元素为一个对象，形如{addr:0x1000,tag:'tag'}。
/// @brief 获取所有hook点列表
/// @function Hooker.getHooks()

/// 判断指定地址是否有hook。
/// @brief 判断指定地址是否有hook
/// @param addr 指定的地址
/// @return 如果有hook，返回true，否则false
/// @function Hooker.hasHook(addr)


var Hooker=(function(){
	var dispatchDict={};
	var hookList=[{}];

	function addToList(addr,tag)
	{
		var info={addr:addr,tag:tag};
		for(var i=1;i<hookList.length;i++)
		{
			if(hookList[i]==undefined)
			{
				hookList[i]=info;
				return i;
			}
		}
		hookList.push(info);
		return hookList.length-1;
	}

	function removeFromList(addr)
	{
		for(var i=1;i<hookList.length;i++)
		{
			if(hookList[i].addr==addr)
			{
				delete hookList[i];
				return;
			}
		}
	}

	function checkInfo(addr,hookfunc,tag)
	{
		addr=parseInt(addr);
		if(dispatchDict[addr]!=undefined)
			throw new Error("Can't hook one addr twice!");
		dispatchDict[addr]=hookfunc;
		var ret=native.checkInfoHook(addr);
		if(ret)
		{
			return addToList(addr,tag);
		}
		else
		{
			delete dispatchDict[addr];
			return false;
		}
	}
	function unHook(addr)
	{
		native.unhook(parseInt(addr));
		removeFromList(parseInt(addr));
		delete dispatchDict[addr];
	}
	function unHookById(id)
	{
		if(id!=0 && hookList[id]!=undefined)
		{
			var addr=hookList[id].addr;
			if(addr!=undefined)
			{
				native.unhook(addr);
				delete dispatchDict[addr];
				delete hookList[id];
			}
		}
	}
	function unHookAll()
	{
		for(var addr in dispatchDict)
		{
			native.unhook(parseInt(addr));
		}
		dispatchDict={};
		hookList=[{}];
	}

	function getHooks()
	{
		return hookList.slice(0);
	}

	function hasHook(addr)
	{
		return dispatchDict[addr]?true:false;
	}

	function parseRegs(regStrtPtr)
	{
		regStrt=native.mread(regStrtPtr,4*9);
		arr=utils.Convert.unpack(regStrt,'IIIIIIIII');
		return {
			eflags:arr[0],
			edi:arr[1],
			esi:arr[2],
			ebp:arr[3],
			esp:arr[4],
			ebx:arr[5],
			edx:arr[6],
			ecx:arr[7],
			eax:arr[8]
		};
	}
	function reparseRegs(regsObj)
	{
		return utils.Convert.pack('IIIIIIIII',
			regsObj.eflags,
			regsObj.edi,
			regsObj.esi,
			regsObj.ebp,
			regsObj.esp,
			regsObj.ebx,
			regsObj.edx,
			regsObj.ecx,
			regsObj.eax
		);
	}
	function dispatchCheckFunction(regs,srcAddr)
	{
		if(dispatchDict[srcAddr]!=undefined)
		{
			var regsObj=parseRegs(regs);
			if(dispatchDict[srcAddr](regsObj)==true)
			{
				native.mwrite(regs,reparseRegs(regsObj));
			}
		}
		else if(LogLevel>=2)
		{
			print('unk srcAddr in Hooker: '+srcAddr);
		}
	}

	return {
		checkInfo:checkInfo,
		unHook:unHook,
		unHookById:unHookById,
		unHookAll:unHookAll,
		getHooks:getHooks,
		hasHook:hasHook,
		dispatchCheckFunction:dispatchCheckFunction,
	};

})();
//necessary now

/// 创建一个stdcall、cdecl或thiscall类型的JS函数，该函数可以通过x86指令call直接进行调用。
/// @brief 创建一个可以由汇编代码直接调用的函数
/// @param func 待调用的js函数
/// @param argCnt 该函数需要的参数个数
/// @param type 函数类型，可以为stdcall cdecl thiscall中的一个
/// @return 生成的native函数
/// @remark 生成的函数没有对重入的保护，如果发生递归调用，则会导致程序崩溃。
/// @function Callback.newFunction(func,argCnt,type)

/// 删除一个native JS函数
/// @brief 删除一个native JS函数
/// @param addr 函数地址
/// @function Callback.deleteFunction(addr)

var Callback={
	_dispatchDict:{length:0},
	_addrDict:{},
	_dispatchFunction:function(id,argsPtr,argCnt)
	{
		if(this._dispatchDict[id]==undefined)
			print('unk srcAddr in Callback: '+id);
		else
		{
			var func=this._dispatchDict[id];
			var args=utils.Convert.unpack(native.mread(argsPtr,argCnt*4),'I'.repeat(argCnt));
			return func.apply(this,args);
		}
	},
	newFunction:function(func,argCnt,type)
	{
		var newId=this._dispatchDict.length++;
		this._dispatchDict[newId]=func;
		return native.newCallback(newId,argCnt,type);
	},
	deleteFunction:function(addr)
	{
		native.deleteMem(addr);
		if(this._addrDict[addr]!=undefined)
		{
			delete this._dispatchDict[this._addrDict[addr]];
		}
	}
};

/// 根据函数地址生成__thiscall类型的函数，调用时第一个参数为ecx指针
/// @brief 根据内存地址生成__thiscall函数
/// @param addr 函数地址
/// @return 生成的函数
/// @remark 生成的函数在调用时可传递任意个数的参数，但是若数量错误可能会导致崩溃
function makeThiscallFunction(addr)
{
	return function()
	{
		var args=[addr,'stdcall',{ecx:arguments[0]}];
		for(var i=1;i<arguments.length;i++)
		{
			args.push(arguments[i]);
		}
		return native.callFunction.apply(this,args);
	}
}

/// 根据函数地址生成__stdcall类型的函数。
/// @brief 根据内存地址生成__stdcall函数
/// @param addr 函数地址
/// @return 生成的函数
/// @remark 生成的函数在调用时可传递任意个数的参数，但是若数量错误可能会导致崩溃
function makeStdcallFunction(addr)
{
	return function()
	{
		var args=[addr,'stdcall',{}];
		for(var i=0;i<arguments.length;i++)
		{
			args.push(arguments[i]);
		}
		return native.callFunction.apply(this,args);
	}
}

/// 根据函数地址生成__cdecl类型的函数。
/// @brief 根据内存地址生成__cdecl函数
/// @param addr 函数地址
/// @return 生成的函数
/// @remark 生成的函数在调用时可传递任意个数的参数，但是若数量错误可能会导致崩溃
function makeCdeclFunction(addr)
{
	return function()
	{
		var args=[addr,'cdecl',{}];
		for(var i=0;i<arguments.length;i++)
		{
			args.push(arguments[i]);
		}
		return native.callFunction.apply(this,args);
	}
}

/// @cond
var exp={
	printOneDisasm:printOneDisasm,
	printDisasms:printDisasms,
	makeHookerFuncFromExp:makeHookerFuncFromExp,
	makeSummFuncFromExp:makeSummFuncFromExp,
	Hooker:Hooker,
	Callback:Callback,
	makeCdeclFunction:makeCdeclFunction,
	makeStdcallFunction:makeStdcallFunction,
	makeThiscallFunction:makeThiscallFunction,
};
module.exports=exp;
root.Hooker=Hooker;

/// @endcond
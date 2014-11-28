var native=require('native');
var Convert=require('utils').Convert;
var win32=require('win32');
cloneObject(require('quickfunc'),global);
require('mystring')(global);


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
module.exports.printOneDisasm=printOneDisasm;

function printDisasms(addr,cnt)
{
	var cur=addr;
	while(cur<addr+cnt)
	{
		cur+=printOneDisasm(cur);
	}
	return cur-addr;
}
module.exports.printDisasms=printDisasms;

// var quickObj={};
// Object.defineProperty(quickObj,'chkstk',{get:function(){

// }})

function makeHookerFuncFromExp(exp,useDbgout)
{
	var eles=exp.split('#').map(function(e){return e.split('@')});
	var output;
	if(useDbgout)
		output=win32.dbgOut;
	else
		output=print;

	return function(regs){
		with(regs)
		{
			eles.forEach(function(ele){
				if(ele[1])
				{
					if(eval(ele[1]))
						output(eval(ele[0]));
				}
				else
					output(eval(ele[0]));
			});
		}
	};
}
module.exports.makeHookerFuncFromExp=makeHookerFuncFromExp;

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
		arr=Convert.unpack(regStrt,'IIIIIIIII');
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
		return Convert.pack('IIIIIIIII',
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
/*
module.exports.Hooker={
	dispatchDict:{},
	checkInfo:function(addr,hookfunc)
	{
		addr=parseInt(addr);
		this.dispatchDict[addr]=hookfunc;
		return native.checkInfoHook(addr);
	},
	unHook:function(addr)
	{
		native.unhook(parseInt(addr));
		if(this.dispatchDict[addr]!=undefined)
			delete this.dispatchDict[addr];
	},
	unHookAll:function()
	{
		for(var addr in this.dispatchDict)
		{
			native.unhook(addr);
		}
		this.dispatchDict={};
	},
	parseRegs:function(regStrtPtr)
	{
		regStrt=native.mread(regStrtPtr,4*9);
		arr=Convert.unpack(regStrt,'IIIIIIIII');
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
	},
	reparseRegs:function(regsObj)
	{
		return Convert.pack('IIIIIIIII',
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
	},
	dispatchCheckFunction:function(regs,srcAddr)
	{
		if(this.dispatchDict[srcAddr]!=undefined)
		{
			var regsObj=this.parseRegs(regs);
			if(this.dispatchDict[srcAddr](regsObj)==true)
			{
				native.mwrite(regs,this.reparseRegs(regsObj));
			}
		}
		else if(LogLevel>=2)
		{
			print('unk srcAddr in Hooker: '+srcAddr);
		}
	}
};*/
//necessary now
root.Hooker=module.exports.Hooker=Hooker;


module.exports.Callback={
	_dispatchDict:{length:0},
	_addrDict:{},
	_dispatchFunction:function(id,argsPtr,argCnt)
	{
		if(this._dispatchDict[id]==undefined)
			print('unk srcAddr in Callback: '+id);
		else
		{
			var func=this._dispatchDict[id];
			var args=Convert.unpack(native.mread(argsPtr,argCnt*4),'I'.repeat(argCnt));
			return func.apply(this,args);
		}
	},
	newFunc:function(func,argCnt,type)
	{
		var newId=this._dispatchDict.length++;
		this._dispatchDict[newId]=func;
		return native.newCallback(newId,argCnt,type);
	},
	deleteFunc:function(addr)
	{
		native.deleteMem(addr);
		if(this._addrDict[addr]!=undefined)
		{
			delete this._dispatchDict[this._addrDict[addr]];
		}
	}
};

module.exports.makeThiscallFunction=function (addr)
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

module.exports.makeStdcallFunction=function (addr)
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

module.exports.makeCdeclFunction=function (addr)
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
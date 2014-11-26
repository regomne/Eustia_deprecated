var __time1=(new Date()).getTime();

var LogLevel=5;
var __Path=[_DllPath,_DllPath+'lib\\'];

function print()
{
	for(var i=0;i<arguments.length;i++)
	{
		var arg=arguments[i];
		if(arg!=undefined && (typeof(arg)=='number' || typeof(arg.valueOf())=='number'))
			arguments[i]=arg.toString(16);
	}
	return _Print.apply(this,arguments);
}

function lookupFile(fname)
{
	fname=fname.replace('/','\\');
	if(fname.length>2 &&
		fname[1]!=':' &&
		(fname[0]!='\\' && fname[0]!='/'))
	{
		for(var i=0;i<__Path.length;i++)
		{
			if(_ExistsFile(__Path[i]+fname))
				return __Path[i]+fname;
		}
		return null;
	}
	else
		return fname;
}

function load(fname)
{
	var fullname=lookupFile(fname);
	if(!fullname)
		throw new Error("can't find file: "+fname);
	else
		return _LoadJS(fullname);
}

function cloneToGlobal(mod)
{
	for(var k in mod)
	{
		global[k]=mod[k];
	}	
}

load('module.js');

var native=require('native');
var utils=require('utils');
cloneToGlobal(native);
cloneToGlobal(utils);

/*
load('memory.js');
load('win32.js');


var Hooker=Hooker || {
	dispatchDict:{},
	checkInfo:function(addr,hookfunc)
	{
		addr=parseInt(addr);
		this.dispatchDict[addr]=hookfunc;
		return _CheckInfoHook(addr);
	},
	unHook:function(addr)
	{
		_Unhook(addr);
		if(this.dispatchDict[addr]!=undefined)
			delete this.dispatchDict[addr];
	},
	unHookAll:function()
	{
		for(var addr in this.dispatchDict)
		{
			_Unhook(addr);
		}
		this.dispatchDict={};
	},
	parseRegs:function(regStrtPtr)
	{
		regStrt=mread(regStrtPtr,4*9);
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
				mwrite(regs,this.reparseRegs(regsObj));
			}
		}
		else if(LogLevel>=2)
		{
			print('unk srcAddr in Hooker: '+srcAddr);
		}
	}
};

var Callback=Callback || {
	_dispatchDict:{length:0},
	_addrDict:{},
	_dispatchFunction:function(id,argsPtr,argCnt)
	{
		if(this._dispatchDict[id]==undefined)
			print('unk srcAddr in Callback: '+id);
		else
		{
			var func=this._dispatchDict[id];
			var args=Convert.unpack(mread(argsPtr,argCnt*4),'I'.repeat(argCnt));
			return func.apply(this,args);
		}
	},
	newFunc:function(func,argCnt,type)
	{
		var newId=this._dispatchDict.length++;
		this._dispatchDict[newId]=func;
		return newCallback(newId,argCnt,type);
	},
	deleteFunc:function(addr)
	{
		deleteMem(addr);
		if(this._addrDict[addr]!=undefined)
		{
			delete this._dispatchDict[this._addrDict[addr]];
		}
	}
};

function makeFunction(addr,callType)
{
	return function()
	{
		var args=[addr,callType];
		for(var i=0;i<arguments.length;i++)
		{
			args.push(arguments[i]);
		}
		return callFunction.apply(this,args);
	}
}

function makeThisCallFunction(addr)
{
	return function()
	{
		var args=[addr,'stdcall',{ecx:arguments[0]}];
		for(var i=1;i<arguments.length;i++)
		{
			args.push(arguments[i]);
		}
		return callFunction.apply(this,args);
	}
}

function makeStdcallFunction(addr)
{
	return function()
	{
		var args=[addr,'stdcall',{}];
		for(var i=0;i<arguments.length;i++)
		{
			args.push(arguments[i]);
		}
		return callFunction.apply(this,args);
	}
}

function makeCdeclFunction(addr)
{
	return function()
	{
		var args=[addr,'cdecl',{}];
		for(var i=0;i<arguments.length;i++)
		{
			args.push(arguments[i]);
		}
		return callFunction.apply(this,args);
	}
}

var __time2=(new Date()).getTime();
if(LogLevel>=5)
	print('init time:',(__time2-__time1).toString()+'ms');
delete __time1;
delete __time2;*/
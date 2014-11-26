var native=require('native');
var Convert=require('utils').Convert;
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
		native.unhook(addr);
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
};
//necessary now
root.Hooker=module.exports.Hooker;


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
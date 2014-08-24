var __time1=(new Date()).getTime();

var Global={};
var LogLevel=5;
var scriptPath=[_DllPath];

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

function load(fname)
{
	if(fname.length>2 &&
		fname[1]!=':' &&
		(fname[0]!='\\' && fname[0]!='/'))
	{
		fname=scriptPath.slice(-1)[0]+fname;
	}
	if(LogLevel>=4)
	{
		print(fname,'loading...');
	}
	return _LoadJS(fname);
}

function tryLoad(fname)
{
	try
	{
		load(fname);
	}
	catch(e)
	{
		print('load failed. ',e);
	}
}

load('script\\stubs.js');
load('script\\myString.js');
tryLoad('script\\memory.js');
tryLoad('script\\asm.js');
tryLoad('script\\win32.js');

function displayObject(obj)
{
	function display_(obj,level)
	{
		var indentStr='\t'.repeat(level);
		var s=indentStr+'{\r\n';
		indentStr=indentStr+'\t';
		for(var prop in obj)
		{
			s+=indentStr+prop+': ';
			var val=obj[prop];
			if(typeof(val)!='object')
			{
				s+=val.toString(16)+',\r\n';
			}
			else
			{
				s+='\r\n';
				s+=display_(val,level+1)+',\r\n';
			}
		}
		s+=indentStr.slice(0,-1)+'}';
		return s;
	}
	print(display_(obj,0));
}
var Convert={
	toU32: function(s,off,be)
	{
		if(off==undefined)
			off=0;
		if(be==undefined)
			be=false;
		if(!be)
		{
			return s.charCodeAt(off)+(s.charCodeAt(off+1)<<8)+
				(s.charCodeAt(off+2)<<16)+(s.charCodeAt(off+3)*(1<<24));
		}
		else
		{
			return s.charCodeAt(off+3)+(s.charCodeAt(off+2)<<8)+
				(s.charCodeAt(off+1)<<16)+(s.charCodeAt(off)*(1<<24));
		}
	},
	toU16: function(s,off,be)
	{
		if(off==undefined)
			off=0;
		if(be==undefined)
			be=false;
		if(!be)
		{
			return s.charCodeAt(off)+(s.charCodeAt(off+1)<<8);
		}
		else
		{
			return s.charCodeAt(off+1)+(s.charCodeAt(off)<<8);
		}
	},
	parseTbl:
	{
		'I':{func:function(){return Convert.toU32}, cnt:4},
		'H':{func:function(){return Convert.toU16}, cnt:2},
	},
	unpack: function(s,pat)
	{
		var retArr=[];
		var be=false;
		if(pat[0]=='>' || pat[0]=='<')
		{
			if(pat[0]=='>')
				be=true;
			pat=pat.slice(1);
		}
		var i=0;
		for(var c=0;c<pat.length;c++)
		{
			var cvtf=Convert.parseTbl[pat[c]];
			if(cvtf==undefined)
			{
				throw 'unknown escape character!';
			}
			
			if(i+cvtf.cnt>s.length)
				throw 'str too short!';
			retArr.push(cvtf.func()(s,i,be));
			i+=cvtf.cnt;
		}
		return retArr;
	},
	swap32: function(d)
	{
		s1=d&0xff;
		s2=(d>>>8)&0xff;
		s3=(d>>>16)&0xff;
		s4=(d>>>24)&0xff;
		return (s1*(1<<24))+(s2<<16)+(s3<<8)+s4;
	}
};

var Hooker={
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
		regStrt=_Mread(regStrtPtr,4*9);
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
	dispatchCheckFunction:function(regs,srcAddr)
	{
		if(this.dispatchDict[srcAddr]!=undefined)
			this.dispatchDict[srcAddr](this.parseRegs(regs));
		else if(LogLevel>=2)
			print('unk srcAddr in Hooker: '+srcAddr);
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

function makeClassCallFunction(addr)
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

function makeSimpleFunction(addr,callType)
{
	return function()
	{
		var args=[addr,callType,{}];
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
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

function genFname(fname)
{
	if(fname.length>2 &&
		fname[1]!=':' &&
		(fname[0]!='\\' && fname[0]!='/'))
	{
		fname=scriptPath.slice(-1)[0]+fname;
	}
	return fname;
}

function load(fname)
{
	fname=genFname(fname);
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
tryLoad('script\\cryptojs.js');

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
	
	fromU32:function(i,be)
	{
		var s;
		if(be==undefined)
			be=false;
		if(be)
		{
			s=String.fromCharCode((i>>>24)&0xff)+
				String.fromCharCode((i>>>16)&0xff)+
				String.fromCharCode((i>>>8)&0xff)+
				String.fromCharCode(i&0xff);
		}
		else
		{
			s=String.fromCharCode((i)&0xff)+
				String.fromCharCode((i>>>8)&0xff)+
				String.fromCharCode((i>>>16)&0xff)+
				String.fromCharCode((i>>>24)&0xff);
		}
		return s;
	},
	
	fromU16:function(i,be)
	{
		var s;
		if(be==undefined)
			be=false;
		if(be)
		{
			s=String.fromCharCode((i>>>8)&0xff)+
				String.fromCharCode(i&0xff);
		}
		else
		{
			s=String.fromCharCode((i)&0xff)+
				String.fromCharCode((i>>>8)&0xff);
		}
		return s;
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
	unpackN:function(s,fmt,names)
	{
		if (names==undefined)
		{
			names=[];
		}
		var rslt=[];
		var sp=0;
		for(var i=0;i<fmt.length;i++)
		{
			if(sp>=s.length)
			{
				throw 'str is not long enough!';
			}
			var f=fmt[i];
			switch (f) {
				case 'I':
					rslt.push([names[i],Convert.toU32(s,sp)]);
					sp+=4;
				break;
				case 'H':
					rslt.push([names[i],Convert.toU16(s,sp)]);
					sp+=2;
				break;
				case 'B':
					rslt.push([names[i],s.charCodeAt(sp)]);
					sp+=1;
				break;
				default:
					throw 'unknown escape character: '+f;
			}
		}
		return rslt;
	},

	pack:function(pat)
	{
		var s=[];
		var be=false;
		if(pat[0]=='>' || pat[0]=='<')
		{
			if(pat[0]=='>')
				be=true;
			pat=pat.slice(1);
		}
		for(var i=0;i<pat.length;i++)
		{
			var c=pat[i];
			var v=arguments[i+1];
			if(v==undefined)
				throw 'args not enough';
			switch(c)
			{
				case 'I':
					s.push(Convert.fromU32(v,be));
				break;
				case 'H':
					s.push(Convert.fromU16(v,be));
				break;
				default:
					throw 'unknown escape charactor';
			}
		}
		return s.join('');
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
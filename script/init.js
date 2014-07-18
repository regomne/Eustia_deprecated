var Global={};
var LogLevel=7;

function apHash(str)
{
    var dwHash = 0;    
    
    for (var dwIndex = 0; dwIndex < str.length; dwIndex++)
    {
		var c=str.charCodeAt(dwIndex);
        dwHash ^= ( ((dwIndex & 1) == 0) ? 
            ((dwHash <<  7) ^ (c) ^ (dwHash >>> 3)) : 
        (~((dwHash << 11) ^ (c) ^ (dwHash >>> 5))) );
    }
    
    dwHash = (dwHash & 0x7fffffff);
    return dwHash;
	
}
function calcExeMemHash(address)
{
	function u32(addr)
	{
		return Convert.toU32(_Mread(addr,4));
	}
	function u16(addr)
	{
		return Convert.toU16(_Mread(addr,2));
	}
	function calcExeModule(addr)
	{
		var sig=_Mread(addr,2);
		if(sig!='MZ')
		{
			return -1;
		}
		addr=addr+u32(addr+0x3c);
		sig=_Mread(addr,2);
		if(sig!='PE')
		{
			return -1;
		}
		
		var size=24+u16(addr+0x14);
		return apHash(_Mread(addr,size));
	}
	
	var h=calcExeModule(address);
	if(h>0)
		return h;
	return apHash(_Mread(address,0x1000));
}

function getNewExecuteMemory(blocks1,blocks2)
{
	function getExecute(blocks)
	{
		var exes=[];
		
		var PAGE_EXECUTE_READ=0x20;
		var PAGE_EXECUTE_READWRITE=0x40;
		
		for(var i=0;i<blocks.length;i++)
		{
			if((blocks[i].protect&(PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE))!=0)
			{
				exes.push(blocks[i]);
			}
		}
		return exes;
	}
	
	var exe1=getExecute(blocks1);
	print('之前的可执行内存数量: ',exe1.length);
	var exe2=getExecute(blocks2);
	print('之后的可执行内存数量: ',exe2.length);
	
	var newExes=[];
	var newChanges=[];
	
	var j=0;
	for(var i=0;i<exe1.length;i++)
	{
		(function(addr,size)
		{
			for(;j<exe2.length;j++)
			{
				if(exe2[j].baseAddress<addr)
				{
					newExes.push(exe2[j]);
					j++;
				}
				else if(exe2[j].baseAddress==addr)
				{
					if(exe2[j].regionSize!=size)
					{
						newChanges.push(exe2[j]);
					}
					j++;
					break;
				}
				else
					break;
			}
		})(exe1[i].baseAddress,exe1[i].regionSize);
	}
	while(j<exe2.length)
	{
		newExes.push(exe2[j++]);
	}
	
	newExes.map(function(mm){mm.hash=calcExeMemHash(mm.baseAddress)});
	return {newExes:newExes,newChanges:newChanges};
}

String.prototype.repeat=function(n)
{
	var a=[];
	while(a.length<n)
	{
		a.push(this);
	}
	return a.join('');
};
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
				s+=val.toString()+',\r\n';
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

function displayMemInfo(obj)
{
	obj.map(function(mm)
	{
		print('start: ',mm.baseAddress,'\t\tsize: ',mm.regionSize,'\t\thash: ',mm.hash);
	});
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
		if(off=undefined)
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
		else if(LogLevel>=4)
			print('unk srcAddr in Hooker: '+srcAddr);
	}
};

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
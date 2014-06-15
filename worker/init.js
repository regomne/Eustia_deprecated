var Global={};
var LogLevel=7;

function hookFunc1(regs)
{
	print('eax: '+regs.eax);
	
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
	toU16: function(s,be)
	{
		if(be==undefined)
			be=false;
		if(!be)
		{
			return s.charCodeAt(0)+(s.charCodeAt(1)<<8);
		}
		else
		{
			return s.charCodeAt(1)+(s.charCodeAt(0)<<8);
		}
	},
	parseTbl:
	{
		'I':{func:Convert.toU32, cnt:4},
		'H':{func:Convert.toU16, cnt:2},
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
		for(var c in pat)
		{
			var cvtf=this.parseTbl[pat[c]];
			if(cvtf==undefined)
			{
				throw 'unknown escape character!';
			}
			
			if(i+cvtf.cnt>s.length)
				throw 'str too short!';
			retArr.push(cvtf.func(s,i,be));
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
		this.dispatchDict[addr]=hookfunc;
		return _CheckInfoHook(addr);
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

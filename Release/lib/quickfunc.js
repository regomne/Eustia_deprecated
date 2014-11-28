var Convert=require('utils').Convert;
var native=require('native');
var mread=native.mread;
var mwrite=native.mwrite;
var memory=require('memory');

var exp={

hex:function(num)
{
	return parseInt(num).toString(16);
},

u32:function(addr)
{
	return Convert.toU32(mread(addr,4));
},
u16:function(addr)
{
	return Convert.toU16(mread(addr,2));
},
u8:function(addr)
{
	return mread(addr,1).charCodeAt(0);
},

wu32:function(addr,i)
{
	mwrite(addr,Convert.fromU32(i));
},
wu16:function(addr,i)
{
	mwrite(addr,Convert.fromU16(i));
},
wu8:function(addr,i)
{
	mwrite(addr,String.fromCharCode(i&0xff));
},

ustr:function(addr,cnt)
{
	var s=[];
	if(cnt==undefined)
	while(true)
	{
		var c=exp.u16(addr);
		if(c==0)
			break;
		addr+=2;
		s.push(String.fromCharCode(c));
	}
	else
	{
		while(cnt--)
		{
			var c=exp.u16(addr);
			addr+=2;
			s.push(String.fromCharCode(c));
		}
	}
	return s.join('');
},
astr:function(addr)
{
	var s=[];
	while(true)
	{
		var c=mread(addr,1);
		if(c=='\0')
			break;
		addr++;
		s.push(c);
	}
	return s.join('');
},
stlString:function(addr)
{
	var curLen=exp.u32(addr+0x10);
	var maxLen=exp.u32(addr+0x14);
	if(maxLen<=7)
		return exp.ustr(addr,curLen);
	else
		return exp.ustr(exp.u32(addr),curLen);
},

dmem:memory.printMem,

chkstk:function(regs)
{
	//displayObject(regs);
	var s='check: [esp] '+exp.u32(regs.esp).toString(16)+', [ebp]';
	var curebp=regs.ebp;
	var i=0;
	while(i<12)
	{
		var last;
		try{last=exp.u32(curebp+4);curebp=exp.u32(curebp);}
		catch(e){break;}
		// if(i==0)
		// 	dumpMemory(-1,last&0xfffff000,0x1000,'c:/1.bin');
		s=s+last.toString(16)+', ';
		i++;
	}
	return s;
},

};

module.exports=exp;
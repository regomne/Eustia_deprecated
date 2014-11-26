var Convert=require('utils').Convert;
var native=require('native');
var mread=native.mread;
var mwrite=native.mwrite;

var exp={
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
		var c=this.u16(addr);
		if(c==0)
			break;
		addr+=2;
		s.push(String.fromCharCode(c));
	}
	else
	{
		while(cnt--)
		{
			var c=this.u16(addr);
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
	var curLen=this.u32(addr+0x10);
	var maxLen=this.u32(addr+0x14);
	if(maxLen<=7)
		return this.ustr(addr,curLen);
	else
		return this.ustr(this.u32(addr),curLen);
}
};

module.exports=exp;
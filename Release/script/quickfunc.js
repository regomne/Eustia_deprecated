function u32(addr)
{
	return Convert.toU32(mread(addr,4));
}
function u16(addr)
{
	return Convert.toU16(mread(addr,2));
}
function u8(addr)
{
	return mread(addr,1).charCodeAt(0);
}

function wu32(addr,i)
{
	mwrite(addr,Convert.fromU32(i));
}
function wu16(addr,i)
{
	mwrite(addr,Convert.fromU16(i));
}
function wu8(addr,i)
{
	mwrite(addr,String.fromCharCode(i&0xff));
}

function ustr(addr,cnt)
{
	var s=[];
	if(cnt==undefined)
	while(true)
	{
		var c=u16(addr);
		if(c==0)
			break;
		addr+=2;
		s.push(String.fromCharCode(c));
	}
	else
	{
		while(cnt--)
		{
			var c=u16(addr);
			addr+=2;
			s.push(String.fromCharCode(c));
		}
	}
	return s.join('');
}
function astr(addr)
{
	var s=[];
	while(true)
	{
		var c=_Mread(addr,1);
		if(c=='\0')
			break;
		addr++;
		s.push(c);
	}
	return s.join('');
}
function stlString(addr)
{
	var curLen=u32(addr+0x10);
	var maxLen=u32(addr+0x14);
	if(maxLen<=7)
		return ustr(addr,curLen);
	else
		return ustr(u32(addr),curLen);
}
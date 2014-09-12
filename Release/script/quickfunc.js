function u32(addr)
{
	return Convert.toU32(_Mread(addr,4));
}
function u16(addr)
{
	return Convert.toU16(_Mread(addr,2));
}
function u8(addr)
{
	return _Mread(addr,1).charCodeAt(0);
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

function ustr(addr)
{
	var s=[];
	while(true)
	{
		var c=u16(addr);
		if(c==0)
			break;
		addr+=2;
		s.push(String.fromCharCode(c));
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
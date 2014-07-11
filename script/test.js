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

function unpack(s,fmt,names)
{
	if (typeof(names)=='undefined')
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
}

function CheckRecv(regs)
{
	//print(regs.esp,'entered');
	if(u32(regs.esp)<=0x2000000)
	{
		print(u32(regs.esp));
	}
}
function HookWsRecv(){Hooker.checkInfo(_GetAPIAddress('ws2_32.recv'),function(regs){CheckRecv(regs)})}
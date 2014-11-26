var native=require('native');

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

function printDisasms(addr,cnt)
{
	var cur=addr;
	while(cur<addr+cnt)
	{
		cur+=printOneDisasm(cur);
	}
	return cur-addr;
}


function printHex(buff,size,addr)
{
	var i=0;
	while(i<buff.length && i<size)
	{
		var s='';
		if(addr!=undefined)
			s=(addr+i).toString(16).rjust(8,'0')+'  ';
		for(var j=0;j<16;j++)
		{
			s+=buff.charCodeAt(i).toString(16).rjust(2,'0')+' ';
			if(j==7)
				s+=' ';
			i++;
			if(i>=buff.length || i>=size)
				break;
		}
		print(s);
	}
}

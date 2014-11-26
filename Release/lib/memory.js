var Convert=require('utils').Convert;
var native=require('native');
require('mystring')(global);

var exp={};
function calcExeMemHash(address)
{
	function u32(addr)
	{
		return Convert.toU32(native.mread(addr,4));
	}
	function u16(addr)
	{
		return Convert.toU16(native.mread(addr,2));
	}
	function calcExeModule(addr)
	{
		var sig=native.mread(addr,2);
		if(sig!='MZ')
		{
			return -1;
		}
		addr=addr+u32(addr+0x3c);
		sig=native.mread(addr,2);
		if(sig!='PE')
		{
			return -1;
		}
		
		var size=24+u16(addr+0x14);
		return apHash(native.mread(addr,size));
	}
	
	var h=calcExeModule(address);
	if(h>0)
		return h;
	return apHash(native.mread(address,0x1000));
}
exp.calcExeMemHash=calcExeMemHash;

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
				while(j<exe2.length && exe2[j].baseAddress<addr)
				{
					newExes.push(exe2[j]);
					j++;
				}
				if(exe2[j].baseAddress==addr)
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
exp.getNewExecuteMemory=getNewExecuteMemory;

function dumpAllMem(handle,dir)
{
	mm=native.getMemoryBlocks(handle);
	mm.forEach(function(m){
		var low=m.protect&0xff;
		var high=m.protect&(~0xff);
		if((low!=0 && low!=1) && (high!=0x100))
		{
			var ret=native.dumpMemory(handle,m.baseAddress,m.regionSize,dir+'\\'+m.baseAddress.toString(16)+'.bin');
			if(!ret)
			{
				print("can't read mem:",m.baseAddress,'protect:',m.protect);
			}
		}
	});
	print('dump complete.');
}
exp.dumpAllMem=dumpAllMem;

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
exp.apHash=apHash;

function displayMemInfo(obj)
{
	obj.map(function(mm)
	{
		print('start: ',mm.baseAddress,'\t\tsize: ',mm.regionSize,'\t\thash: ',mm.hash);
	});
}
exp.displayMemInfo=displayMemInfo;

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
exp.printHex=printHex;

function printMem(addr,size)
{
	printHex(native.mread(addr,size),size,addr);
}
exp.printMem=printMem;

module.exports=exp;
/// @file memory.js
/// @brief 内存操作相关的模块

/// @cond
var Convert=require('utils').Convert;
var native=require('native');
require('mystring')(global);

var exp={};
/// @endcond

/// 计算可执行内存块hash（rpcs方案202号包使用）
/// @brief 计算可执行内存块hash
/// @param address 内存块起始地址
/// @return 内存块hash
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

/// 对比两个内存块快照，获得第二个内存块快照中新增和改变过大小的可执行模块。
/// @brief 获得新增的可执行模块
/// @param blocks1 原始的可执行内存块快照，可使用native.getMemoryBlocks函数获得
/// @param blocks2 新的可执行内存块快照，可使用native.getMemoryBlocks函数获得
/// @return 返回新增和改变的内存模块对象{newExes:{},newChanges{}}
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

/// dump指定进程的全部内存到指定目录中。其中每个内存块使用一个文件存储，文件以内存起始地址命名。
/// @brief dump指定进程的全部内存到指定目录中
/// @param handle 目标进程的句柄
/// @param dir 要存放内存块的目录
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

/// 计算一个字符串的hash（rpcs方案中的hash算法）。
/// @brief 计算一个字符串的hash
/// @param str 输入字符串
/// @return 计算的hash
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

/// 显示一个内存块快照的内容。
/// @brief 显示一个内存块快照的内容
/// @param obj 内存块快照对象，可使用native.getMemoryBlocks函数获得
function displayMemInfo(obj)
{
	obj.map(function(mm)
	{
		print('start: ',mm.baseAddress,'\t\tsize: ',mm.regionSize,'\t\thash: ',mm.hash);
	});
}

/// 以十六进制方式打印一个指定字符串的内容，可指定长度和是否显示内存地址。
/// @brief 以十六进制方式打印一个指定字符串的内容
/// @param buff 字符串
/// @param size 最长打印的长度
/// @param addr 要显示的内存地址，即假定字符串起始位置是在该地址处。
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

/// 以十六进制方式打印指定内存处的内容。
/// @brief 以十六进制方式打印指定内存处的内容
/// @param addr 内存地址
/// @param size 内存长度
function printMem(addr,size)
{
	if(!size)
		size=0x80;
	printHex(native.mread(addr,size),size,addr);
}

/// @cond
exp.printMem=printMem;
exp.calcExeMemHash=calcExeMemHash;
exp.printHex=printHex;
exp.displayMemInfo=displayMemInfo;
exp.apHash=apHash;
exp.dumpAllMem=dumpAllMem;
exp.getNewExecuteMemory=getNewExecuteMemory;

module.exports=exp;

/// @endcond
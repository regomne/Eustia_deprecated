/// @file
/// @brief 操作map文件的模块

/// @cond
var native=require('native');
require('mystring')(global);
/// @endcond

/// 读取一个map文件，将其中的函数名--地址表读取进来，以对象的形式存储。对于以数字开头的函数名，会把其第一个数字改为_。
/// @brief 读取map文件
/// @param fname map文件名
/// @return 函数名--地址对象
function readMapFile(fname)
{
	var ls=native.readText(fname).split('\r\n');
	var bgLine=-1;
	for(var i=0;i<ls.length;i++)
	{
		if(ls[i].trim().startswith('Address'))
		{
			bgLine=i+1;
			break;
		}
	}
	if(bgLine==-1)
		throw "maybe not a map file!";
	
	var maps={};
	for(var i=bgLine;i<ls.length;i++)
	{
		if(ls[i].trim().startswith('Static'))
			break;
		
		s=ls[i].trim().split(/[ ]+/);
		if(s.length>=3)
		{
			var key=s[1];
			if(key[0]>='0' && key[0]<='9')
			{
				key='_'+key.slice(1);
			}
			maps[key]=parseInt(s[2],16);
		}
	}
	return maps;
}

/// @cond
module.exports.readMapFile=readMapFile;

/// @endcond
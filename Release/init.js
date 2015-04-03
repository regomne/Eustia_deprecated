/// @file init.js
/// @brief 初始化，root空间

/// 日志等级
var LogLevel=4;

/// 快速require，不重复查找同名模块文件
var FastRequire=true; 

/// global对象
var root=global;

/// 文件搜寻路径
var __Path=[_DllPath,_DllPath+'lib\\'];

/// 控制台输出或dbgView输出（根据_OutputState不同），接受任意个参数。\n
/// _OutputState第0位和第1位分别代表是否在控制台和dbgView中输出。
/// @brief 文本输出
/// @param ... 任意参数，其中整数会以十六进制输出
function print()
{
	for(var i=0;i<arguments.length;i++)
	{
		var arg=arguments[i];
		if(arg!=undefined && (typeof(arg)=='number' || typeof(arg.valueOf())=='number'))
			arguments[i]=arg.toString(16);
	}
	return _Print.apply(this,arguments);
}

/// 在__Path中查找指定的文件名并返回全名。
/// @brief 查找文件名
/// @param fname 文件名，相对或绝对路径均可
/// @return string 文件全名，或null（查找失败时）
function lookupFile(fname)
{
	fname=fname.replace(/\//g,'\\');
	if(fname.length>2 &&
		fname[1]!=':' &&
		(fname[0]!='\\' && fname[0]!='/'))
	{
		for(var i=0;i<__Path.length;i++)
		{
			if(_ExistsFile(__Path[i]+fname))
				return __Path[i]+fname;
		}
		return null;
	}
	else
		return fname;
}

/// 读取一个js文件，并在本模块中直接加载，其中的代码在本模块中的Context内运行。
/// @brief 在本模块中加载js
/// @param fname js文件名
/// @return js文件的返回值
/// @remark 由于会污染本模块的命名空间，所以应该慎用
function load(fname)
{
	var fullname=lookupFile(fname);
	var ret;
	if(!fullname)
		throw new Error("can't find file: "+fname);
	else
		ret=_LoadJS(fullname);

	if(LogLevel>=3)
		print(fname,'loaded');
	return ret;
}

/// 把一个对象的全部可枚举属性复制到另一个对象内。
/// @brief 复制对象
/// @param src 源对象
/// @param dest 目标对象
/// @remark 本函数进行直接赋值的复制，意即非value的对象将仅复制引用。
function cloneObject(src,dest)
{
	for(var k in src)
	{
		dest[k]=src[k];
	}	
}

/// 载入一个模块。eustia会优先在当前当前js所在目录中搜寻，随后搜索__Path。模块文件名会自动以.js结尾。\n
/// 若同名（指全路径名相同）模块已经被加载过，则默认不会重复加载，而是直接取出缓存的该模块。除非forceReload参数被设置为true。若希望仅通过模块名，而非全路径名进行匹配（可以加快require的速度），请将FastRequire变量设置为true。
/// @brief 载入一个模块
/// @param name 模块名
/// @param forceReload 是否强制重新加载
/// @remark 模块所在的Context是独立的，非显式修改其他模块的行为不会对其他模块产生任何影响。\n
/// 显式修改其他模块的行为包括通过调用者传入的参数修改，修改从上一级模块继承的引用变量的属性，以及对root模块的修改。
/// @return 载入的模块，载入失败会抛出异常
/// @function require(name,forceReload)

/// 载入一个模块，并将模块导出的所有属性全部复制到指定的参数内。通常这里指定的是global变量。
/// @brief 载入一个模块并复制全部属性
/// @param name 模块名
/// @param ggl 目标对象
/// @param reload 是否强制重新加载
function requireAll(name,ggl,reload)
{
	var mod=require(name,reload);
	cloneObject(mod,ggl);
}

load('module.js');

var native=require('native');
var cmdparser=require('cmdparser');
var utils=require('utils');
var asm=require('asm');
var memory=require('memory');
var plugin=require('plugin');

cloneObject(native,global);
cloneObject(utils,global);
cloneObject(asm,global);
cloneObject(memory,global);

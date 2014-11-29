var __time1=(new Date()).getTime();

var LogLevel=4;
var FastRequire=true; //make 'require' load a module without looking for the module file
var root=global;
var __Path=[_DllPath,_DllPath+'lib\\'];

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

function lookupFile(fname)
{
	fname=fname.replace('/','\\');
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

function cloneObject(mod,ggl)
{
	for(var k in mod)
	{
		ggl[k]=mod[k];
	}	
}

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

cloneObject(native,global);
cloneObject(utils,global);
cloneObject(asm,global);
cloneObject(memory,global);

var __time2=(new Date()).getTime();
if(LogLevel>=4)
	print('init time:',(__time2-__time1).toString()+'ms');
delete __time1;
delete __time2;
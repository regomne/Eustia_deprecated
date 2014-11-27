var asm=require('asm');
var native=require('native');
require('mystring')(global);

var Win32={
	_apis:[
		'kernel32.HeapAlloc',
		'kernel32.HeapFree',
		'kernel32.GetProcessHeap',
		'kernel32.VirtualProtect',
		'kernel32.LoadLibraryA',
		'kernel32.OpenProcess',
		'winmm.timeGetTime',
		'kernel32.OutputDebugStringA',
		'kernel32.OutputDebugStringW',
	],
	
	init:function()
	{
		that=this;
		this._apis.forEach(function(name)
		{
			that.add(name);
		});
	},
	
	add:function(name)
	{
		var newName=name;
		if(name.indexOf('.')!=-1)
			newName=name.split('.')[1];
		if(name[0]=='#')
			this[newName]=asm.makeCdeclFunction(native.getAPIAddress(name.slice(1)));
		else
			this[newName]=asm.makeStdcallFunction(native.getAPIAddress(name));
	}
};

Win32.init();

Win32.newMem=function(size)
{
	var ret=Win32.HeapAlloc(Win32.GetProcessHeap(),0,size);
	if(ret==0)
		throw "alloc meme error!";
	return ret;
}

Win32.deleteMem=function(ptr)
{
	Win32.HeapFree(Win32.GetProcessHeap(),0,ptr);
}

Win32.dbgOut=function(format){
	var args=[];
	for(var i=1;i<arguments.length;i++)
		args.push(arguments[i]);
	var s=(format.format.apply(format,[args])+'\0').encode();
	Win32.OutputDebugStringW(s);
};

module.exports=Win32;
var Win32={
	_apis:[
		'kernel32.HeapAlloc',
		'kernel32.HeapFree',
		'kernel32.GetProcessHeap',
		'kernel32.VirtualProtect',
		'kernel32.LoadLibraryA',
		'winmm.timeGetTime',
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
		this[newName]=makeStdcallFunction(getAPIAddress(name));
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

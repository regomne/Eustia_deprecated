var memory=require('memory');
var native=require('native');
var Win32=require('win32');
var asm=require('asm');
require('mystring')(global);

var Parser_js_disStartAddress=0x401000;
var Parser_js_dataStartAddress=0x400000;

var ShortCmdTable={
	cm1: 'mm1=native.getMemoryBlocks()',
	cm2: 'mm2=native.getMemoryBlocks();rslt=memory.getNewExecuteMemory(mm1,mm2);memory.displayMemInfo(rslt.newExes)',
	lf: 'load("myfunc.js")',
	llib: 'Win32.LoadLibraryA("{0}")',
	us: 'asm.printOneDisasm(parseInt("{0}",16))',
	u: function(addr)
		{
			if(addr==undefined)
				addr=Parser_js_disStartAddress;
			else
				addr=parseInt(addr,16);
			
			addr+=asm.printDisasms(addr,50);
			Parser_js_disStartAddress=addr;
		},
	d: function(addr,size)
		{
			if(addr==undefined)
				addr=Parser_js_dataStartAddress;
			else
				addr=parseInt(addr,16);
			
			if(size==undefined)
				size=0x80;
			else
				size=parseInt(addr,16);
			
			memory.printMem(addr,size);
			Parser_js_dataStartAddress=addr+size;
		},
};

function ParseShortCmd(cmd,ggl)
{
	var str=cmd.split(' ');
	cmd=[];
	str.forEach(function(ele){
		if(ele!='')
			cmd.push(ele);
	});
	
	var opr=ShortCmdTable[cmd[0]];
	if(typeof(opr)=='string')
	{
		eval(opr.format(cmd.slice(1)));
	}
	else if(typeof(opr)=='function')
	{
		opr.apply(this,cmd.slice(1));
	}
	else
	{
		print("Unknown short cmd!");
	}
}

module.exports.parseShortCmd=ParseShortCmd;

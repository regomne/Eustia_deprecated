var memory=require('memory');
var native=require('native');
var win32=require('win32');
var asm=require('asm');
//cloneToGlobal(require('quickfunc'));
require('mystring')(global);

var Parser_js_disStartAddress=0x401000;
var Parser_js_dataStartAddress=0x400000;

function addCmd()
{
	if(arguments.length>=2)
		ShortCmdTable[arguments[0]]=arguments[1];
	else
	{
		var obj=arguments[0];
		for(var k in obj)
		{
			ShortCmdTable[k]=obj[k];
		}
	}
}
module.exports.addCmd=addCmd;

var ShortCmdTable=(function (){


	return {

	'.cm1': 'mm1=native.getMemoryBlocks()',
	'.cm2': 'mm2=native.getMemoryBlocks();rslt=memory.getNewExecuteMemory(mm1,mm2);memory.displayMemInfo(rslt.newExes)',
	'.lf': 'load("myfunc.js")',
	'.llib': 'win32.LoadLibraryA("{0}")',
	'.hook' : function(addr){
		if(arguments.length<2)
		{
			print('must have addr and exps!');
			return;
		}

		var symbol='';
		if(!(addr[0]>='0' && addr[0]<='9'))
		{
			symbol=addr;
			addr=native.getAPIAddress(symbol);
			if(!addr)
			{
				print("Can't find symbol:",symbol);
				return;
			}
		}
		else
		{
			addr=parseInt(addr,16);
		}

		if(asm.Hooker.hasHook(addr))
		{
			print(addr,'already hooked, please clear first');
			return;
		}

		var args=[];
		for(var i=1;i<arguments.length;i++)
			args.push(arguments[i]);
		var exp=args.join(' ');

		var ret=asm.Hooker.checkInfo(addr,asm.makeHookerFuncFromExp(exp),symbol);
		if(ret)
			print('id',ret,'hooked.');
		else
			print('hook failed.');
	},
	'.hl': function(){
		var list=asm.Hooker.getHooks();
		for(var i=1;i<list.length;i++)
		{
			var item=list[i];
			if(item!=undefined)
				print(i,item.addr,item.tag?item.tag:'');
		}
	},
	'.hc': function(id){
		asm.Hooker.unHookById(id);
	},

	load:'load("{0}")',
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
})();

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

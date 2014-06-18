var excludeList=[1,4,0x3f,46,6,7,34,35,5,57,65,26];

function MyHook_(regs)
{
	var t=_Mread(regs.ecx+0x16,1).charCodeAt(0);
	
	if(excludeList.indexOf(t)==-1)
	{
		print('send:',t);
	}
}
function MyHook(regs)
{
	MyHook_(regs);
}
function subFunc(regs)
{
	var t=regs.ecx;
	if(excludeList.indexOf(t)==-1)
	{
		print('recv:',regs.ecx);
	}
}
function MyHook2(regs)
{
	subFunc(regs);
}
function HookRecv(){Hooker.checkInfo(0x183cea5,MyHook2)}
function HookSend(){Hooker.checkInfo(0x1838afc,MyHook)}

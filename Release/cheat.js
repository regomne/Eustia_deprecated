var ROLEBASE=0X2B110E0;
var TEAM_TYPE_OFFSET=0x82c;
var CATTACK_OFFSET=0xf8d044;
var CCOLLISION_OFFSET=0x103b4a6;
var GETTIME_IAT=0x22fb5cc;

function newCattack(regs)
{
	var player=u32(ROLEBASE);
	if(u32(regs.esi+TEAM_TYPE_OFFSET)==100)
	{
		mwrite(regs.ebp-0x18,Convert.fromU32(u32(regs.ebp-0x18)*20))
	}
}

function HookSuperDamage(){Hooker.checkInfo(CATTACK_OFFSET,function(regs){return newCattack(regs)})}

function newCollision(regs)
{
	var player=u32(ROLEBASE);
	{
		if(regs.edi==player)
			regs.eax=0;
		else if(u32(regs.esi+TEAM_TYPE_OFFSET)==u32(player+TEAM_TYPE_OFFSET))
			regs.eax=1;
	}
	return true;
}

function HookWudi(){Hooker.checkInfo(CCOLLISION_OFFSET,function(regs){return newCollision(regs)})}

var startTime=0;
function newGetTime()
{
	if(startTime==0)
		startTime=Win32.timeGetTime();
	return startTime+(Win32.timeGetTime()-startTime)*2;
}

function HookTime()
{
	var gt=Callback.newFunc(newGetTime,0,'stdcall');
	var oldProt=Win32.newMem(4);
	Win32.VirtualProtect(GETTIME_IAT,4,0x40,oldProt);
	mwrite(GETTIME_IAT,Convert.fromU32(gt));
	Win32.deleteMem(oldProt);
}

function cheat(){HookSuperDamage();HookWudi();HookTime();}
function closeCheat(){Hooker.unHook(CATTACK_OFFSET);Hooker.unHook(CCOLLISION_OFFSET)}
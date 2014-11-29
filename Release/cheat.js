load('dnf_def.js');
load('quickfunc.js');
var ROLEBASE=PLAYER_INFO_INDEX;
var CATTACK_OFFSET=0x00FCDCA4;
var CCOLLISION_OFFSET=0x0107c436; //ccollisionresult
var SETHP_FUNC=0x01010AC0; //hpcheat-XX
var Convert=require('utils').Convert;
//var GETTIME_IAT=0x22fb5cc;
//var GAME_SCORE_BASE=0x2AAECE4; //be4ad9d7d33448f56e49ce2047d0521a

function newCattack(regs)
{
	var player=u32(ROLEBASE);
	var team=u32(regs.esi+TEAM_TYPE_OFFSET);
	if((team==100 || team==50 || team==110 || team!=0)
		/* && regs.ebx==player*/
	)
	{
		var old;
		old=2500000;
		//old=u32(regs.ebp-ATTACK_DAMAGEHP_OFFSET);
		mwrite(regs.ebp-ATTACK_DAMAGEHP_OFFSET,Convert.fromU32(old*20))
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
		;
	}
	return true;
}

function HookWudi(){Hooker.checkInfo(CCOLLISION_OFFSET,function(regs){return newCollision(regs)})}

function newSethp(regs)
{
	var player=u32(ROLEBASE);
	if(regs.ecx==player)
	{
		wu32(regs.esp+4,100000);
	}
}
function HookSethp(){Hooker.checkInfo(SETHP_FUNC,function(regs){return newSethp(regs)})}

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

function cheat(){HookSuperDamage();HookWudi();HookSethp()}
function closeCheat(){Hooker.unHook(CATTACK_OFFSET);Hooker.unHook(CCOLLISION_OFFSET)}
function fen(){wu32(u32(GAME_SCORE_BASE)+0x19f,0xf423f)}

var Consts={
	
};

function u32(addr)
{
	return Convert.toU32(_Mread(addr,4));
}
function u16(addr)
{
	return Convert.toU16(_Mread(addr,2));
}
function u8(addr)
{
	return _Mread(addr,1).charCodeAt(0);
}

function unpack(s,fmt,names)
{
	if (typeof(names)=='undefined')
	{
		names=[];
	}
	var rslt=[];
	var sp=0;
	for(var i=0;i<fmt.length;i++)
	{
		if(sp>=s.length)
		{
			throw 'str is not long enough!';
		}
		var f=fmt[i];
		switch (f) {
			case 'I':
				rslt.push([names[i],Convert.toU32(s,sp)]);
				sp+=4;
			break;
			case 'H':
				rslt.push([names[i],Convert.toU16(s,sp)]);
				sp+=2;
			break;
			case 'B':
				rslt.push([names[i],s.charCodeAt(sp)]);
				sp+=1;
			break;
			default:
				throw 'unknown escape character: '+f;
		}
	}
	return rslt;
}

function CheckVal(regs)
{
	print(regs.esi,regs.edi);
}
function ChkVal(addr){Hooker.checkInfo(addr,function(regs){CheckVal(regs)})}

//var excludeList=[0,1,4,0x3f,46,5,6,7,34,35,57,65,26,12,13,18,101];
var excludeList=[1,4,34,63];
function MyHook_(regs)
{
	var t=u32(regs.esp+4)
	
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
function HookRecv(){Hooker.checkInfo(0x18821f5,MyHook2)}
function HookSend(){Hooker.checkInfo(0x0187D140,MyHook)}


function GetDamagedHP2(regs,val)
{
	print('hp: ',u32(regs.ebp-0x18),val);
}

function CheckAttackCount(regs)
{
	print('multiAcc:', u16(0x031d4efa));
}

function HookCattack(){
	var table={
		0xf07eb4:GetDamagedHP2,
		0x3818587:CheckAttackCount,
		0x3812a6f:CheckAttackCount,
	};
	for(var addr in table)
	{
		Hooker.checkInfo(parseInt(addr),function(regs){table[addr](regs);});
	}
	return function(){for(var addr in table){Hooker.unHook(parseInt(addr))}};
}

function GetCnt(addr,regs)
{
	print(addr,' entered. espVal: ',u32(regs.esp),'esp+4',u32(regs.esp+4),'esp+8',u32(regs.esp+8),'last:',u32(u32(regs.ebp)+4));
}
//function HookDecHp(){Hooker.checkInfo(0xf1e270,function(regs){GetCnt(regs)})}
function HookChkStack(addr)
{
	Hooker.checkInfo(addr,function(regs)
	{
		GetCnt(addr,regs);
	});
}

function GetSetStateInfo(regs)
{
	var base=u32(0x29d6d00);
	var t=u16(base+0x1c);
	var id=u16(base+0x1e);
	var st1=u8(base+0x20);
	var st2=(u32(base+0x21))&0xffffffff;
	if(t==0x21 && u32(regs.esi)==0x2324f6c)
	{
		print('t: ',t,' id: ',id,' st1: ',st1,' st2: ',st2);
	}
}
function HookSetStateInfo(){Hooker.checkInfo(0xfc20ef,function(regs){GetSetStateInfo(regs)})}

function GetUseSkillInfo(regs)
{
	var skillId=u32(regs.esp+4);
	print('obj: ',regs.ecx,' SkillId: ',skillId);
}
function HookUseSkillInfo(){Hooker.checkInfo(0xf90a40,function(regs){GetUseSkillInfo(regs)})}

function GetCSPkgCode(regs)
{
	var pkgType=u16(regs.ebp+0xc);
	var pkgBuffer=u32(regs.ebp+0x10);
	print('1',pkgType);
	
	if(pkgType==37)
	{
		var dataSize=u16(regs.ebp+0x14);
		var p=pkgBuffer;
		for(var i=0;i<dataSize;i+=9)
		{
			print('id: ',u32(p),'Skid: ',u8(p+4),'hitcnt:',u16(p+5),'atkcnt:',u16(p+7));
			p+=9;
		}
	}
	else if(pkgType==50)
	{
		var dataSize=u16(regs.ebp+0x14);
		var p=pkgBuffer;
		for(var i=18;i<dataSize;i+=6)
		{
			print('idx: ',u16(p+i),'val: ',u32(p+i+2));
			
		}
	}
	else if(pkgType==202)
	{
		print('202!');
	}
}
function GetCSPkgCode2(regs)
{
	var pkgType=u16(regs.esp+0x8);
	var pkgBuffer=u32(regs.esp+0xc);
	var dataSize=u16(regs.esp+0x10);
	print('2',pkgType);
	
	if(pkgType==37)
	{
		var p=pkgBuffer;
		for(var i=0;i<dataSize;i+=9)
		{
			print('id1: ',u32(p),'Skid: ',u8(p+4),'hitcnt:',u16(p+5),'atkcnt:',u16(p+7));
			p+=9;
		}
	}
	else if(pkgType==50)
	{
		var p=pkgBuffer;
		for(var i=18;i<dataSize;i+=6)
		{
			print('idx1: ',u16(p+i),'val: ',u32(p+i+2));
			
		}
	}
	else if(pkgType==202)
	{
		print('202.1!');
	}
}
function HookPackCSPkg(addr){Hooker.checkInfo(addr,function(regs){GetCSPkgCode(regs)})}
function HookPackCSPkg2(addr){Hooker.checkInfo(addr,function(regs){GetCSPkgCode2(regs)})}

function GetScanRpcp(regs)
{
	print('ret: ',u32(regs.esp));
	var item=u32(regs.esp+4);
	var names=['idx','off1','off2','off3','off4','len','isDec'];
	var fmt='HIIIIHB';
	var vals=unpack(_Mread(item,21),fmt,names);
	var s='';
	for(var i=0;i<vals.length;i++)
	{
		s+=vals[i][0]+': '+vals[i][1]+' ';
	}
	print(s);
}

function HookScanRpcp(addr){Hooker.checkInfo(addr,function(regs){GetScanRpcp(regs)})}

function CheckEffect(regs)
{
	var objType=u32(regs.esp+8);
	if(objType==0xf000)
	{
		var effObj=u32(regs.esp+0x10);
		print('uid:',u16(regs.esp+0xc),'effId:',u32(effObj+0x35c));
	}
}
function HookCheckEffect(addr){Hooker.checkInfo(addr,function(regs){CheckEffect(regs)})}

function getPtrByUidAndType(uid,type)
{
	var t=u32(0x2c82dd8+0x24);
	t=u32(t+0x20a020);
	t=u32(t+0x90);
	return callFunction(0x1035180,'stdcall',{ecx:t},type,uid);
}

function CheckVectorAdd(regs)
{
	var mapObj=u32(u32(0x2a95fe8)+0xb8);
	var vec=regs.ecx;
	var newElem=u32(u32(regs.ebp+8));
	if((vec==mapObj+0xc || vec==mapObj+0x34 || vec==mapObj+0xc0) && u32(newElem+0x94)==0xf000)
	{
		print('eff added');
	}
}
function HookCheckVector(addr){Hooker.checkInfo(addr,function(regs){CheckVectorAdd(regs)})}


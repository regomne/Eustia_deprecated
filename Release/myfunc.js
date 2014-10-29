var Consts={
	
};

load('protocols.js');
load('dnf_def.js');

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
	if (names==undefined)
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

function printUnpack(vals)
{
	var s='';
	for(var i=0;i<vals.length;i++)
	{
		s+=vals[i][0]+': '+vals[i][1]+' ';
	}
	return s;
}

function CheckVal(regs)
{
	//displayObject(regs);
	var s='check: '+u32(regs.esp).toString(16)+',';
	var curebp=regs.ebp;
	var i=0;
	while(i<12)
	{
		var last;
		try{last=u32(curebp+4);curebp=u32(curebp);}
		catch(e){break;}
		if(i==0)
			dumpMemory(-1,last&0xfffff000,0x1000,'c:/1.bin');
		s=s+last.toString(16)+', ';
		i++;
	}
	print(s);
	
}
function ChkVal(addr){Hooker.checkInfo(addr,function(regs){CheckVal(regs)})}

function newRunChk(regs,addr)
{
	print(addr,'runed');
    // var item=u32(regs.esp+4);
    // printMem(item,0x30);
    // var item=u32(regs.esp);
    // if(u16(item)==701)
    //     print(regs.eax);
}
function RunChk(addr){Hooker.checkInfo(addr,function(regs){newRunChk(regs,addr)})}

function GetCnt(addr,regs,dist)
{
	var esp=regs.esp+dist;
	//print(addr,' entered. espVal: ',u32(esp),'esp+XX',u32(esp+4+0x18),'esp+4',u32(esp+4),'esp+8:',u32(esp+8));
	printMem(esp,0x10);
	
}
//function HookDecHp(){Hooker.checkInfo(0xf1e270,function(regs){GetCnt(regs)})}
function HookChkStack(addr,dist)
{
	Hooker.checkInfo(addr,function(regs)
	{
		GetCnt(addr,regs,dist);
	});
}

var excludeList=[1,4,6,7,12,13,34,46,63,101];
var includeList=[];
function MyHook_(regs)
{
	var t=u32(regs.esp+4)
	
	if(excludeList.indexOf(t)==-1)
	{
		if(includeList.length!=0 && includeList.indexOf(t)==-1)
			return;
		print('send:',t);
	}
}
function subFunc(regs)
{
	var t=regs.ecx;
	if(excludeList.indexOf(t)==-1)
	{
		if(includeList.length!=0 && includeList.indexOf(t)==-1)
			return;
		if(t==5)
		{
			var st=u32(regs.ebx+0xb);
            var buff=regs.ebx;
			print(u32(buff+0xb),u16(buff+7),u16(buff+9));
		}
		print('recv:',regs.ecx);
	}
}
function HookRecv(){Hooker.checkInfo(0x18f4c35,function(regs){subFunc(regs)})}
function HookSend(){Hooker.checkInfo(0x0187D140,function(regs){MyHook_(regs)})}

var LastTime=0;
function myUdpSend(regs)
{
	function filter(pid,buff)
	{
		var exList=[];
		if(exList.indexOf(pid)==-1)
			return true;
		return false;
	}
	var udpBuff=u32(UDP_PACKET_MANAGER_OBJECT)+UDP_PACKET_BUFF_OFFSET;
	if(udpBuff<1000)
	{
		return;
	}
	
	var type=u8(udpBuff+UDP_PACKET_FLAG_OFFSET);
	if(type!=2)
	{
		return;
	}
	
	var pkId=u16(udpBuff+UDP_PACKET_ID_OFFSET);
	if(filter(pkId,udpBuff))
	{
		if(pkId==5)
		{
			if(u16(udpBuff+7)==0x111 && u8(udpBuff+0xb)==8)
			{
				var subId=u8(udpBuff+0x12);
				if(LastTime==0) LastTime=(new Date()).getTime();
				var curTime=(new Date()).getTime();
				print('subid:',subId,'interval:',curTime-LastTime);
				LastTime=curTime;
			}
		}
	}
}
function hookUdpSend(){Hooker.checkInfo(0x01883fe7,function(regs){myUdpSend(regs)})}

function myTcpSend(regs)
{
	function sendMsgFilter(pid,buff)
	{
		var exList=[0];
		if(pid<0x372 && exList.indexOf(pid)==-1) return true;
		return false;
	}

	var tcpBuff=u32(SEND_TCP_PACKET_BUFF_OFFSET);
	if(tcpBuff==0)
	{return;}
	realBuff=tcpBuff+SEND_TCP_PACKET_REALDATA_OFFSET;
	var buffSize=u32(tcpBuff+SEQ_OFFSET)+0xd;
	var pkId=u16(realBuff+SEND_TCP_PACKET_ID_OFFSET);
	if(sendMsgFilter(pkId,realBuff))
	{
		print('Send id:',pkId,CmdName[pkId],buffSize);
		if(pkId==0x13)
		{
			printMem(realBuff,buffSize);
		}
	}
}
function hookTcpSend(){Hooker.checkInfo(TCP_SENDBUFF_FUNC,function(regs){myTcpSend(regs)})}

function myTcpRecv(regs)
{
	function recvMsgFilter(pid,buff)
	{
		var exList=[2,3,6,0x76];
		if(pid<0x375 && exList.indexOf(pid)==-1) return true;
		return false;
	}
	
	var tcpBuff=u32(regs.esp+8);
	var pkId=u16(tcpBuff+DNFEVENT_PACKET_ID_OFFSET);
	var pkSize=u16(tcpBuff+DNFEVENT_PACKET_SIZE_OFFSET);
	if(recvMsgFilter(pkId,tcpBuff))
	{
		print('Recv id:',pkId,NotiName[pkId],'size:',pkSize);
		if(pkId==14)
		{
			printMem(tcpBuff,pkSize);
		}
	}
}
//addr: DBG_Hook_TcpRecv=E54368
function hookTcpRecv(){Hooker.checkInfo(0xE542E0,function(regs){myTcpRecv(regs)})}

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


function GetSetStateInfo(regs,fmt,names)
{
	var base=u32(0x2a12f98);
	var t=u16(base+0x16);
	var vals=unpack(_Mread(base+0x1c,100),fmt,names);
	var s='';
	for(var i=0;i<vals.length;i++)
	{
		s+=vals[i][0]+': '+vals[i][1]+' ';
	}
	print('t:',t,s);
}
function Hookudp(addr,fmt,names){Hooker.checkInfo(addr,function(regs){GetSetStateInfo(regs,fmt,names)})}

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
	var pkgType=u16(regs.esp+0xc+0x8);
	var pkgBuffer=u32(regs.esp+0xc+0xc);
	var dataSize=u16(regs.esp+0xc+0x10);
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

function getMemoryBlock(mm,addr)
{
	var cm;
	mm.forEach(function(m)
	{
		if(m.baseAddress<=addr && m.baseAddress+m.regionSize>addr)
		{
			cm=m;
			return;
		}
	});
	return cm;
}

function getMemoryBases(mm,addr)
{
	var cms=[];
	mm.forEach(function(m)
	{
		if(m.allocationBase==addr)
		{
			cms.push(m);
		}
	});
	return cms;
}

function listMemoriesWithAddr(addr)
{
	var mm=getMemoryBlocks(-1);
	var ss=getMemoryBlock(mm,addr);
	if(ss==undefined)
	{
		print("can't find addr.");
		return;
	}
	ss=getMemoryBases(mm,ss.allocationBase);
	if(ss==undefined)
	{
		print("can't find base.");
		return;
	}
	displayObject(ss);
}

function CheckEsp(regs)
{
	var esp=regs.esp;
	print('Check esp begin');
	for(var i=0;i<100;i++)
	{
		var val=u32(esp);
		if(val<0x5000000 && val>0x400000)
		{
			var last=0;
			try{last=u32(u32(esp-4));}
			catch(e){}
			print(esp,':',val, ':',last);
		}
		esp+=4;
	}
	print('Check esp end.');
}
function HookCheckEsp(addr){Hooker.checkInfo(addr,function(regs){CheckEsp(regs)})}

function GetItemObject()
{
	try
	{
		var vec1=u32(u32(u32(u32(0x2a986bc)+0x18)+0x344)+0x13c);
		var start=u32(vec1+0x3c);
		var end=u32(vec1+0x40);
		var cnt=(end-start)/4;
		for (var i=0;i<cnt;i++)
		{
			print(i,u32(start+i*4));
		}
	}
	catch(e)
	{
		print('err');
	}
}

var Checked=null;
var CheckedBuff;
function myCheckVal2(regs,off,addr,serial)
{
	var esp=regs.ebp+off;
	var pkId=u32(esp) & 0xff;
	var buff=u32(esp+4);
	var buffLen=u16(esp+8);
	
	if(serial==1)
	{
		if(Checked==null)
			return;
		var newBuff=mread(buff,buffLen);
		if(newBuff!=CheckedBuff)
		{
			print('not fit:',pkId.toString(10));
			if(pkId==37)
			{
				printHex(CheckedBuff,CheckedBuff.length);
				print('');
				printHex(newBuff,newBuff.length);
		  }
		}
		Checked=null;
	}
	else if(serial==0)
	{
		CheckedBuff=mread(buff,buffLen);
		Checked=pkId;
	}
	//print(pkId);
// 	if(pkId==9)
//	{
//	print(buff,buffLen);	printMem(buff,buffLen);
//	}
}
function checkVal2(addr,off,serial){Hooker.checkInfo(addr,function(regs){myCheckVal2(regs,off,addr,serial)})}

function hookVectorPush(){Hooker.checkInfo(0x018CA523,function(regs){
	if(regs.ecx==u32(u32(0x2b0e000)+0x30a4))
	{
		CheckVal(regs);
	}
})}

function getFingerprint(addr,size)
{
	var step=size/128;
	var res1=[];
	var cur=addr;
	var sum=0;
	for(var i=0;i<128;i++)
	{
		var code=_Mread(cur,1).charCodeAt(0);
		cur+=step;
		res1.push(code);
		sum+=code;
	}
	var ave=sum/128;
	
	var rslt=[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];
	
	for(var i=0;i<128;i++)
	{
		if(res1[i]>=ave)
		{
			var nbyte=Math.floor(i/8);
			var nbit=i%8;
			
			rslt[nbyte] |= (1<<nbit);
		}
	}
	return rslt;
}

function dumpEquip(folder)
{
    var player=u32(PLAYER_INFO_INDEX);
    for(var i=0xa;i<=TITLE_CARD_INDEX;i++)
    {
        var p=u32(player+i*4+INVENTORY_BASE_OFFSET);
        if(p)
        {
            print(p);
            dumpMemory(-1,p,0x2800,folder+'\\'+i.toString(16)+'_'+p.toString(16)+'.bin');
        }
    }
}
function DecryptVal(val)
{
    var tmp=u32(val);
    var decBaseTbl=u32(DECRYPT_BASETABLE_OFFSET);
    var firstIdx=(tmp>>>16);
    var tableFirst=u32(decBaseTbl+firstIdx*4+DECRYPT_BASETABLE_RET1_OFFSET);
    var secondIdx=tmp&0xffff;
    tmp=u32(tableFirst+secondIdx*4+DECRYPT_BASETABLE_RET2_OFFSET);
    
    var key=((tmp&0xffff)<<16)|(tmp&0xffff);
    return key^(u32(val+8));
}

function enumSkill()
{
	var data={};
	var skillStart=u32(PLAYER_INFO_INDEX)+0x49a0;
	for(var c=0;c<=0xfe;c++)
	{
		var obj=u32(skillStart+c*4);
		if(obj!=0)
		{
			var id=DecryptVal(obj+SKILL_ID_OFFSET);
			var exId=DecryptVal(obj+EX_SKILLDATAINDEX_OFFSET);
			var quan=DecryptVal(obj+0x6d8+12);
			var name=stlString(obj+0xc);
			print('id:',id,'exId',exId,'quan',quan,'name:',name);
		}
	}
}
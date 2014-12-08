var native=require('native');
var mread=native.mread;
var mwrite=native.mwrite;
cloneObject(require('quickfunc'),global);
require('mystring')(global);
var asm=require('asm');
var Hooker=asm.Hooker;
cloneObject(require('memory'),global);

load('protocols.js');
load('dnf_def.js');

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
		// if(i==0)
		// 	dumpMemory(-1,last&0xfffff000,0x1000,'c:/1.bin');
		s=s+last.toString(16)+', ';
		i++;
	}
	print(s);
	
}
function ChkVal(addr){Hooker.checkInfo(addr,function(regs){CheckVal(regs)})}

var ImgMap={};
var ImgCnt=0;
function newRunChk(regs,addr)
{
	//Win32.OutputDebugStringW(u32(u32(regs.esp+4)+4));
  var img=ustr(u32(regs.esp+4))
  if(img.toLowerCase().endswith('missile.img'))
    ;
    // var item=u32(regs.esp);
    // if(u16(item)==701)
    //     print(regs.eax);
}
function RunChk(addr){Hooker.checkInfo(addr,function(regs){newRunChk(regs,addr)})}

function newRunChk2(regs,addr)
{
	print(addr,'run');
	printMem(regs.esp,0x18);
    //printMem(u32(regs.esp+4),0x10);
    // var item=u32(regs.esp);
    // if(u16(item)==701)
    //     print(regs.eax);
}
function RunChk2(addr){Hooker.checkInfo(addr,function(regs){newRunChk2(regs,addr)})}

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

var excludeList=[1,0x3f];
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
	var t=u16(regs.ebx+1);
	var buff=regs.ebx;
	var size=u32(regs.ebp+RECV_UDP_PACKET_SIZE_OFFSET);
	if(excludeList.indexOf(t)==-1)
	{
		if(includeList.length!=0 && includeList.indexOf(t)==-1)
			return;
		// if(t==5)
		// {
		// 	var st=u32(regs.ebx+0xb);
  //           var buff=regs.ebx;
		// 	print(u32(buff+0xb),u16(buff+7),u16(buff+9));
		// }
		//print('recv:',t);

    if(t==0x4)
    {
    	print('recv4');
      var flag=u32(buff+0x12);
      var pflag=parseMoveFlag(flag);
      pflag.forEach(function(ele){print(ele)});
      var curp=0x16;
      for(var i=0;i<2;i++)
      {
        for(var j=0;j<3;j++)
        {
          if(!pflag[j][i])
          {
            print(native.toFloatp(buff+curp).toString())
            curp+=4;
          }
        }
      }
      for(var i=2;i<4;i++)
      {
        for(var j=0;j<3;j++)
        {
          if(!pflag[j][i])
          {
            print((u32(buff+curp)&0xffffffff).toString())
            curp+=4;
          }
        }
      }
    }
	}
}
function HookRecv(){Hooker.checkInfo(0x0191c593,function(regs){subFunc(regs)})}
function HookSend(){Hooker.checkInfo(0x01917B80,function(regs){MyHook_(regs)})}

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
	var udpBuff=regs.edi;
	if(udpBuff<1000)
	{
		return;
	}
	
	var pkId=u16(udpBuff+UDP_PACKET_ID_OFFSET);
  var sendMgr=u32(UDP_PACKET_MANAGER_OBJECT);
  var buffEnd=u32(sendMgr+UDP_PACKET_BUFF_END_OFFSET);
  var pkSize=buffEnd-udpBuff;
	if(filter(pkId,udpBuff))
	{
    //print('id:',pkId,'size',pkSize);
    if(pkId==0x4)
    {
      var buff=udpBuff;
      var objType=u16(udpBuff+0x8);
      var objUid=u16(udpBuff+0xa);
      //printMem(buff,0x10);
      var flag=u32(udpBuff+0x12);
      var pflag=parseMoveFlag(flag);
      pflag.forEach(function(ele){print(ele)});
      var curp=0x16;
      /*
      for(var i=0;i<2;i++)
      {
        for(var j=0;j<3;j++)
        {
          if(!pflag[j][i])
          {
            print(native.toFloatp(buff+curp).toString())
            curp+=4;
          }
        }
      }
      for(var i=2;i<4;i++)
      {
        for(var j=0;j<3;j++)
        {
          if(!pflag[j][i])
          {
            print((u32(buff+curp)&0xffffffff).toString())
            curp+=4;
          }
        }
      }
      */
      var x,y;
      if(!pflag[0][0])
      {
        x=toFloatp(buff+curp)
        curp+=4;
      }
      if(!pflag[1][0])
      {
        y=toFloatp(buff+curp);
        curp+=4;
      }
      if(objType==0x211 && (x!=undefined || y!=undefined))
        CacheMoveInfo(objUid,x,y);

    }
    else if(pkId==0x3f)
    {
      //print('3f being sent.');
      wu32(sendMgr+UDP_PACKET_BUFF_END_OFFSET,udpBuff);
    }
    else if(pkId==0x13)
    {
      print('hitobject,size:',pkSize);
      var att=u16(udpBuff+0x9);
      var uatt=u16(udpBuff+0xd);
      // wu16(udpBuff+9,uatt);
      // wu16(udpBuff+0xd,att);
      //mwrite(udpBuff+0xf,'\0'.repeat(12));
      //printMem(udpBuff,pkSize);
    }
	}
}
function hookUdpSend(){Hooker.checkInfo(0x01918EE7,function(regs){myUdpSend(regs)})}

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
		if(pkId==0x2a)
		{
			printMem(tcpBuff,pkSize);
		}
	}
}

function hookTcpRecv(){Hooker.checkInfo(0xE5e980,function(regs){myTcpRecv(regs)})}

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
	var mm=native.getMemoryBlocks(-1);
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
	utils.displayObject(ss);
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

function getFingerprint(addr,size)
{
	var step=size/128;
	var res1=[];
	var cur=addr;
	var sum=0;
	for(var i=0;i<128;i++)
	{
		var code=mread(cur,1).charCodeAt(0);
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


function getImgInfo(name)
{
  function getHash()
  {
    var hash=0;
    for(var i=0;i<name.length;i++)
    {
      hash=(hash*0x1003f+name.charCodeAt(i))&0xffffffff;
    }
    return hash;
  }

  var h=getHash();
  var getF=asm.makeThiscallFunction(0x1dd36f0);
  var ret=getF(0x02C6A430+0x1c,h,[(name+'\0').encode()],0);
  if(ret==0)
    return null;
  var off=u32(ret+8);
  var size=u32(ret+0xc);
  var npkname=stlString(u32(ret+0x10)+4);
  return [off,size,npkname];
}

function parseMoveFlag(flag)
{
  flag=flag&0xffffffff;
  var x,y,z;
  var v=[flag>>26,
      (flag>>20)&0x3f,
      (flag>>14)&0X3f,
      (flag>>8)&0x3f];
  x=v.map(function(ele){return (ele&3)});
  y=v.map(function(ele){return ((ele>>2)&3)});
  z=v.map(function(ele){return ((ele>>4)&3)});
  x.push((flag>>5)&1);
  y.push((flag>>5)&2);
  z.push((flag>>5)&4);
  return [x,y,z,((flag>>4)&1)];
}

//key: uid, val:{curx,cury,dist}
var cachedMonster={};
function CacheMoveInfo(uid,x,y)
{
  print('cache: ',uid);
  function getDist(x1,y1,x2,y2)
  {
    if(x1==undefined)
    {
      if(y2==undefined || y1==undefined)
        return 0;
      return Math.abs(y2-y1);
    }
    else if(y1==undefined)
    {
      if(x2==undefined || x1==undefined)
        return 0;
      return Math.abs(x2-x1);
    }
    else
    {
      if(x2==undefined)
        return Math.abs(y2-y1);
      if(y2==undefined)
        return Math.abs(x2-x1);
      return Math.sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
    }
  }
  var mst=cachedMonster[uid];
  if(!mst)
  {
    mst={curx:x,cury:y,dist:0};
    cachedMonster[uid]=mst;
    return;
  }
  mst.dist+=getDist(mst.curx,mst.cury,x,y);
  if(x!=undefined)
    mst.curx=x;
  if(y!=undefined)
    mst.cury=y;
}

function getObjTypeByPtr(ptr)
{
  var type=u32(ptr+OBJECT_TYPE_OFFSET);
  if(type!=TYPE_APC && type!=TYPE_STRIKER && type!=TYPE_MONSTER)
  {
    type=u32(ptr+TYPE_OFFSET);
  }
  return type;
}

function getTopCaster(att)
{
  var top=att;
  while(top!=0)
  {
    topType=getObjTypeByPtr(top)
    if(topType!=TYPE_SPRITE && ((topType&TYPE_EFFECT)!=TYPE_EFFECT))
    {
      break;
    }
    top=u32(att+CASTER_OFFSET);
  }
  return top;
}

function enumMap(map,func)
{
    function midTrav(ele)
    {
        if(ele!=0 && u8(ele+0x15)!=1)
        {
            midTrav(u32(ele));
            func(u32(ele+0xc),u32(ele+0x10));
            midTrav(u32(ele+8));
        }
    }
    
    var endMap=u32(map+8);
    var head=u32(endMap+4);
    midTrav(head);
}

function getEffectMap(regs)
{
  var att=u32(regs.ebp+8);
  var top=getTopCaster(att);
  if(top!=att)
  {
    //effect
    var id=u32(att+EFFECTID_OFFSET)
    enumMap(0x2dcbc28,function(key,val){
      if(key==id)
        print(id,ustr(val));
    });
  }
}
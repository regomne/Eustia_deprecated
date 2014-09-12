var Analyzer={
    init:function(startAddr,region,newBranch)
    {
        this.startAddr=startAddr;
        
        this.region=region;
        
        this.newBranchProc=newBranch;
    },

    isRegion:function(addr)
    {
        return this._isInRange(this.region,addr);
    },
    
    getSwitch:function(switchAddr)
    {
        function parseJmpTable()
        {
            var table2Addr=parseInt(mo[1]);
            if(jmpTabel1==null)
            {
                if(cmpVal==null)
                {
                    throw 'cmpVal is null '+curAddr;
                }
                for(var i=0;i<=cmpVal;i++)
                {
                    var dAddr=u32(table2Addr+i*4);
                    if(switchTable[i-valDist]!=undefined)
                        throw (i-valDist)+' exists! in '+curAddr;
                    
                    switchTable[i-valDist]=dAddr;
                }
            }
            else
            {
                var minTabel={};
                
                for(var i=0;i<jmpTabel1.length;i++)
                {
                    var idx=jmpTabel1.charCodeAt(i);
                    if(minTabel[idx]!=undefined)
                        continue;
                    
                    var dAddr=u32(table2Addr+idx*4);
                    
                    if(defaultAddrs[dAddr]!=undefined)
                        continue;
                    
                    if(switchTable[i-valDist]!=undefined)
                        throw (i-valDist)+' exists! in '+curAddr;
                    
                    switchTable[i-valDist]=dAddr;
                    minTabel[idx]=1;
                }
            }
        }
        var switchTable={};
        
        var curAddr=switchAddr;
        var defaultAddrs={};
        var switchReg='eax'; //switch使用的key val
        var cmpVal=null; //cmp指令比较的val
        var valDist=0; //key val的增减值
        var branchTable=[]; //条件跳转的分支缓存
        var jmpTabel1=null;
        
        print('cur branch: ',curAddr);
        
        var pat=this._instPattern;
        var state='init';
        while(true)
        {
            if(!this.isRegion(curAddr))
            {
                throw 'out of region';
            }
            
            var processed=true; //如果下面设置为false则重新分析本条指令，否则下一句
            var disasm=disassemble(curAddr);
            //print(curAddr,disasm.string,state);
            var distr=disasm.string;
            if(state=='init')
            {
                var mo=pat.cmpPat.exec(distr);
                if(mo!=null && mo[1]==switchReg)
                {
                    cmpVal=parseInt(mo[2]);
                    if(isNaN(cmpVal))
                    {
                        state='over'; //这里没有比较寄存器，否则是一个识别defalut的时机
                    }
                    else
                        state='cmp';
                }
                else if((mo=pat.addPat.exec(distr))!=null && mo[1]==switchReg)
                {
                    valDist+=(parseInt(mo[2])|0);
                    state='addsub';
                }
                else if((mo=pat.subPat.exec(distr))!=null && mo[1]==switchReg)
                {
                    valDist-=(parseInt(mo[2])|0);
                    state='addsub';
                }
                /*else if((mo=this._instPattern.jxxPat.exec(disasm.string))!=null)
                {
                    var dst=parseInt(mo[2]);
                    if(mo[1]=='jmp' && (dst==outAddr || dst==defaultAddr))
                    {
                        popBranch();
                        continue;
                    }
                }*/
                else
                {
                    if(distr.indexOf(switchReg)!=-1)
                        throw 'unk inst with eax! '+distr;
                    state='over';
                }
            }
            else if(state=='cmp')
            {
                var mo;
                if((mo=pat.addPat.exec(distr))!=null && mo[1]==switchReg)
                {
                    valDist+=(parseInt(mo[2])|0);
                    state='addsub';
                }
                else if((mo=pat.subPat.exec(distr))!=null && mo[1]==switchReg)
                {
                    valDist-=(parseInt(mo[2])|0);
                    state='addsub';
                }
                else if((mo=pat.jxxPat.exec(distr))!=null)
                {
                    var destAddr=parseInt(mo[2]);
                    if(!this.isRegion(destAddr))
                        throw "out region in jxx ,"+curAddr;
                    
                    if(mo[1]=='je')
                    {
                        if(cmpVal==null || (switchTable[cmpVal]!=undefined && switchTable[cmpVal]!=destAddr))
						{
							print(cmpVal,switchTable[cmpVal]);
                            throw cmpVal+' exists! in '+curAddr;
						}
                        switchTable[cmpVal]=destAddr;
                    }
                    else if(mo[1]=='jg')
                    {
                        branchTable.push(destAddr);
                    }
                    else if(mo[1]=='ja')
                    {
                        defaultAddrs[destAddr]=(defaultAddrs[destAddr]||0)+1;
                    }
                    else
                        throw 'unk jxx in '+curAddr;
                }
                else if((mo=pat.cmpPat.exec(distr))!=null && mo[1]==switchReg)
                {
                    cmpVal=parseInt(mo[2]);
                    if(isNaN(cmpVal))
                    {
                        state='over';
                    }
                }
                else if((mo=pat.movzxPat.exec(distr))!=null && mo[1]==switchReg)
                {
                    var cnt=cmpVal;
                    jmpTabel1=mread(parseInt(mo[2]),cnt+1);
                    state='table';
                }
                else if((mo=pat.jmpTabelPat.exec(distr))!=null)
                {
                    parseJmpTable();
                    state='over';
                }
                else
                {
                    if(distr.indexOf(switchReg)!=-1)
                        throw 'unk inst with eax! '+distr;
                    state='over';
                }
            }
            else if(state=='shortTable')
            {
                var mo;
                if((mo=pat.movzxPat.exec(distr))!=null && mo[1]==switchReg)
                {
                    var cnt=cmpVal;
                    jmpTabel1=mread(parseInt(mo[2]),cnt+1);
                    state='table';
                }
                else if((mo=pat.jmpTabelPat.exec(distr))!=null)
                {
                    parseJmpTable();
                    state='over';
                }
                else if((mo=pat.jxxPat.exec(distr))!=null)
                {
                    var destAddr=parseInt(mo[2]);
                    if(!this.isRegion(destAddr))
                        throw "out region in jxx ,"+curAddr;
                    
                    if(mo[1]=='ja' || mo[1]=='jnbe')
                    {
                        defaultAddrs[destAddr]=(defaultAddrs[destAddr]||0)+1;
                    }
                    else
                        throw 'unk jxx in shortTable '+curAddr;
                }
                else
                {
                    throw 'unk inst in '+curAddr;
                }
            }
            else if(state=='table')
            {
                var mo;
                if((mo=pat.jmpTabelPat.exec(distr))!=null)
                {
                    parseJmpTable();
                    state='over';
                }
                else
                {
                    throw 'unk inst in '+curAddr;
                }
            }
            else if(state=='addsub')
            {
                var mo;
                if((mo=pat.jxxPat.exec(distr))!=null)
                {
                    var destAddr=parseInt(mo[2]);
                    if(!this.isRegion(destAddr))
                        throw "out region in jxx ,"+curAddr;
                    
                    if(mo[1]=='je')
                    {
                        if(switchTable[-valDist]!=undefined)
                        {
                            throw (-valDist)+'exists in switch table'+curAddr;
                        }
                        switchTable[-valDist]=destAddr;
						state='init';
                    }
                    else
                    {
                        throw 'unk jxx in addsub'+curAddr;
                    }
                }
                else if((mo=pat.cmpPat.exec(distr))!=null && mo[1]==switchReg)
                {
                    cmpVal=parseInt(mo[2]);
                    if(isNaN(cmpVal))
                    {
                        throw 'unk cmp val in addsub '+curAddr;
                    }
					state='shortTable';
                }
				else
					throw 'unexpected inst in addsub '+curAddr;
            }
            else if(state=='over')
            {
                if(branchTable.length==0)
                    break;
                curAddr=branchTable.shift();
                if(!this.isRegion(curAddr))
                {
                    print('over region! branch:',curAddr);
                    continue;
                }
				print('new branch entered: ',curAddr);
                state='init';
                valDist=0;
                jmpTable1=null;
                cmpVal=null;
				continue;
            }
			else
				throw 'unk state: '+state;
            
            if(processed)
                curAddr+=disasm.length;
        }
		
		displayObject(defaultAddrs);
		
        return switchTable;
    },
    
    getCallsFromBranch:function(addr,calls,outAddr,exceptAddr)
    {
        if(u16(addr)!=0x3d80 || u16(addr+7)!=0x850f)
            throw 'unk branch start!';
        
        var disasm=disassemble(addr+7);
        var mo=this._instPattern.jxxPat.exec(disasm.string);
        var firstAddr=addr=parseInt(mo[2]);
        
        var state='init';
        while(true)
        {
            if(!this.isRegion(addr))
            {
                print('branch over region. ',firstAddr);
                break;
            }
            
            disasm=disassemble(addr);
            if(state=='init')
            {
                if((mo=this._instPattern.jmpPat.exec(disasm.string))!=null)
                {
                    var dstAddr=parseInt(mo[1]);
                    if(isNaN(dstAddr))
                        throw 'cvt jmp addr failed! '+addr;
                    
                    if(this._isInRange(outAddr,dstAddr))
                    {
                        print('branch from',firstAddr,' size:',addr-firstAddr);
                        break;
                    }
                }
                else if((mo=this._instPattern.callPat.exec(disasm.string))!=null)
                {
                    var dstAddr=parseInt(mo[1]);
                    if(isNaN(dstAddr))
                        throw 'cvt call addr failed! '+addr;
                    
                    if(calls[dstAddr]==undefined)
                        calls[dstAddr]=1;
                    else
                        calls[dstAddr]++;
                }
            }
            
            addr+=disasm.length;
        }
    },
    
    getAllCallsFromBranch:function(outAddr,keyDict)
    {
        var calls={};
        for(var k in keyDict)
        {
            print(k);
            this.getCallsFromBranch(keyDict[k],calls,outAddr);
        }
        return calls;
    },
    
    getRegAndCallRelation:function(reg,addr,outAddr,pats,relations)
    {
        if(u16(addr)!=0x3d80 || u16(addr+7)!=0x850f)
            throw 'unk branch start!';
        
        var disasm=disassemble(addr+7);
        var mo=this._instPattern.jxxPat.exec(disasm.string);
        var firstAddr=addr=parseInt(mo[2]);
        
        var state='init';
        var lastCall=0;
        var lastOffset=0;
        var offCnt=0;
        while(true)
        {
            if(!this.isRegion(addr))
            {
                print('branch over region. ',firstAddr);
                break;
            }
            
            disasm=disassemble(addr);

            if(state=='init')
            {
                if((mo=this._instPattern.jmpPat.exec(disasm.string))!=null)
                {
                    var dstAddr=parseInt(mo[1]);
                    if(isNaN(dstAddr))
                        throw 'cvt jmp addr failed! '+addr;
                    
                    if(this._isInRange(outAddr,dstAddr))
                    {
                        print('offCnt:',offCnt);
                        break;
                    }
                }
                else if((mo=this._instPattern.callPat.exec(disasm.string))!=null)
                {
                    var dstAddr=parseInt(mo[1]);
                    if(isNaN(dstAddr))
                        throw 'cvt call addr failed! '+addr;
                    
                    lastCall=dstAddr;
                }
                else if(disasm.string.indexOf(reg)!=-1)
                {
                    if((mo=pats.lea.exec(disasm.string))!=null)
                    {
                        state='lea';
                        lastOffset=parseInt(mo[1]);
                    }
                    else if((mo=pats.mov.exec(disasm.string))!=null)
                    {
                        var offset=parseInt(mo[1]);
                        var num=parseInt(mo[2]);
                        if(!isNaN(num))
                            lastCall=num;
                        
                        if(relations[offset]==undefined)
                        {
                            relations[offset]=[lastCall];
                            offCnt++;
                        }
                        else
                            relations[offset].push(lastCall);
                    }
                    else if((mo=pats.calc.exec(disasm.string))!=null)
                    {
                        var offset=parseInt(mo[2]);
                        if(relations[offset]==undefined)
                        {
                            relations[offset]=[lastCall];
                            offCnt++;
                        }
                        else
                            relations[offset].push(lastCall);
                    }
                    else if((mo=pats.push.exec(disasm.string))!=null)
                    {
                        state='lea';
                        lastOffset=-1;
                    }
                }
            }
            else if(state=='lea')
            {
                if((mo=this._instPattern.jmpPat.exec(disasm.string))!=null)
                {
                    var dstAddr=parseInt(mo[1]);
                    if(isNaN(dstAddr))
                        throw 'cvt jmp addr failed! '+addr;
                    throw 'jmp out with a lea XXX';
                    
                    if(this._isInRange(outAddr,dstAddr))
                    {
                        print('branch from',firstAddr,' size:',addr-firstAddr);
                        break;
                    }
                }
                else if((mo=this._instPattern.callPat.exec(disasm.string))!=null)
                {
                    var dstAddr=parseInt(mo[1]);
                    if(isNaN(dstAddr))
                        throw 'cvt call addr failed '+addr;
                    
                    if(relations[lastOffset]==undefined)
                    {
                        relations[lastOffset]=[dstAddr];
                        offCnt++;
                    }
                    else
                        relations[lastOffset].push(dstAddr);
                    
                    state='init';
                }
                else if(disasm.string.indexOf(reg)!=-1)
                {
                    if((mo=pats.lea.exec(disasm.string))!=null)
                    {
                        state='lea';
                        lastOffset=parseInt(mo[1]);
                        throw 'two lea '+addr.toString(16);
                    }
                    else if((mo=pats.mov.exec(disasm.string))!=null)
                    {
                        var offset=parseInt(mo[1]);
                        var num=parseInt(mo[2]);
                        if(!isNaN(num))
                            lastCall=num;
                        
                        if(relations[offset]==undefined)
                        {
                            relations[offset]=[lastCall];
                            offCnt++;
                        }
                        else
                            relations[offset].push(lastCall);
                    }
                    else if((mo=pats.calc.exec(disasm.string))!=null)
                    {
                        var offset=parseInt(mo[2]);
                        if(relations[offset]==undefined)
                        {
                            relations[offset]=[lastCall];
                            offCnt++;
                        }
                        else
                            relations[offset].push(lastCall);
                    }
                    else if((mo=pats.push.exec(disasm.string))!=null)
                    {
                        throw 'lea & push '+addr.toString(16);
                        state='lea';
                        lastOffset=-1;
                    }
                }
            }
            
            addr+=disasm.length;
        }
    },
    
    getAllOffsetsRelation:function(keymap,outAddr)
    {
        var pats={lea:/lea.*,.*\[edi\+([^\]]+)\]/,
            mov:/mov.*\[edi\+([^\]]+)\],\s*(\w*)/,
            calc:/(add|or).*\[edi\+([^\]]+)\].*/,
            push:/push\s+edi/,
        };
        
        var keyrels={};
        for(var k in keymap)
        {
            var rels={};
            print('get key:',k);
            this.getRegAndCallRelation('edi',keymap[k],outAddr,pats,rels);
            keyrels[k]=rels;
        }
        return keyrels;
    },
    
    _getJxxType:function(s)
    {
        var symbol=null;
        if(s=='ja' || s=='jg' || s=='jnbe')
            symbol='>';
        else if(s=='jb' || s=='jl')
            symbol='<';
        else if(s=='je' || s=='jz')
            symbol='=';
        else if(s=='jle' || s=='jbe')
            symbol=',';
        else if(s=='jae' || s=='jge')
            symbol='.'
        else
            throw 'unk jxx '+s;
        return symbol;
    },
    
    _instPattern:
    {
        cmpPat:/cmp\s+([^,]*),(.*)/,
        jxxPat:/(j\w+)\s*(.*)/,
        addPat:/add\s+([^,]*),(.*)/,
        subPat:/sub\s+([^,]*),(.*)/,
        movzxPat:/movzx\s+\w+, byte ptr \[(\w+)\+([^\]]+)\]/,
        jmpPat:/jmp\s+(0x.*)/,
        callPat:/call\s+(0x.*)/,
        jmpTabelPat:/jmp\s+dword ptr \[([^\+]+)\+\w+\*4\]/,
    },
    
    _funcTbl:
    {
        0xcff070: {type:'pushVecFloat'},//_e562072753298367ce056a17d553f42
        0x1dc6f60: {type:'read str'},//_135464e5a5c44cca0bae3c93c19ef7f
        0x1dc64a0: {type:'read int'},//_231eea78a588c4c2c09ab1f838e7c2b
        0x1e89b20: {type:'get attr index'},//acd016aa25f4388194414e046458dfd4
        0x1f1d9c0: {type:'pushVec4Dword'},//bb94000cb68d17754753bef3888285dd
    },
    
    _isInRange:function(region,addr)
    {
        return region.some(function(reg){return addr>=reg[0] && addr<reg[1]});
    },
};

var findKey=makeThisCallFunction(0x45d100);
var getMapEnd=makeThisCallFunction(0x1db9880);
var isItorEqual=makeThisCallFunction(0x10f47c0);
var Keymap=0x2d73518;

function findScriptKey(key)
{
    function makeUnicodeStr(str)
    {
        var C=CryptoJS;
        return C.enc.Latin1.stringify(C.enc.Utf16LE.parse(str));
    }
    
    var val=-1;
    
    var outbuff=newMem(0x40);
    findKey(Keymap,outbuff,[makeUnicodeStr(key)]);
    var endItor=getMapEnd(Keymap,outbuff+0x20);
    var ret=isItorEqual(outbuff,outbuff+0x20) & 0xff;
    if(ret)
    {
        val=u32(outbuff+8);
        val=u32(val+0x10);
    }
    deleteMem(outbuff);
    
    return val;
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

function getKeysToAddressMap(keyAddr,switchTable)
{
    var map={};
    enumMap(keyAddr,function(ele,num)
    {
        if(switchTable[num]==undefined)
        {
            print(ele,num,'table not found');
        }
        else
        {
            map[ustr(ele)]=switchTable[num];
        }
    });
    return map;
}

function analyRels(rel)
{
    var addrs={};
    var conflicts={};
    var offs={};
    for(var k in rel)
    {
        var kv=rel[k];
        for(var off in kv)
        {
            if(offs[off]!=undefined)
            {
                offs[off].push(k);
                conflicts[off]=1;
            }
            else
            {
                offs[off]=[k];
            }
            
            for(var idx in kv[off])
            {
                var addr=kv[off][idx];
                if(addr<0x400000)
                    continue;
                
                if(addrs[addr]!=undefined)
                {
                    addrs[addr]++;
                }
                else
                {
                    addrs[addr]=1;
                }
            }
        }
    }
    
    for(var conf in conflicts)
    {
        print('conflict: ',parseInt(conf),offs[conf].map(function(ele){return ele.slice(0,-1)}));
    }
    
    print('summary:');
    for(var addr in addrs)
    {
        print(parseInt(addr),addrs[addr]);
    }
}

Analyzer.init(0,[[0x01EF01B0,0x01EFD3E3]],0);
//sw=Analyzer.getSwitch(0x01EF0E17);
//keymap=getKeysToAddressMap(0x2d73518,sw);
//cs=Analyzer.getAllCallsFromBranch([[0x1ef43a9,0x1ef43ac]],keymap)
//rel=Analyzer.getAllOffsetsRelation(keymap,[[0x1ef43a9,0x1ef43ac]]);
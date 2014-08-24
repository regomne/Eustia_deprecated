var Analyzer={
    init:function(startAddr,region,newBranch)
    {
        this.startAddr=startAddr;
        
        if(typeof(region)=='function')
            this.isRegion=region;
        else
        {
            this.isRegion=function(addr)
            {
                return region.some(function(reg){return addr>=reg[0] && addr<reg[1]});
            }
        }
        
        this.newBranchProc=newBranch;
    },

    
    getSwitch:function(switchAddr,defaultAddr,outAddr)
    {
        function popBranch()
        {
            print('branch ',branchTable.shift(),'exited');
            state='init';
            cmpVal=null;
            jmpTabel1=null;
            valDist=0;
        }
        var curAddr=switchAddr;
        var state='init'; //状态机 应该允许混合状态
        var switchReg=['eax']; //switch使用的key val
        var cmpVal=null; //cmp指令比较的val
        var valDist=0; //key val的增减值
        var branchTable=[curAddr]; //条件跳转的分支缓存
        var switchTable={};
        
        var buffVal=[];
        var jmpTabel1=null;
        
        print('cur branch: ',curAddr);
        
        while(true)
        {
            if(branchTable.length==0)
                break;
            curAddr=branchTable[0];
            if(!this.isRegion(curAddr))
            {
                print('over region! branch:',branchTable[0]);
                popBranch();
                continue;
            }
            
            var processed=true; //如果下面设置为false则重新分析本条指令，否则下一句
            var disasm=disassemble(curAddr);
            //print(curAddr,disasm.string,state);
            if(state=='init')
            {
                var mo=this._instPattern.cmpPat.exec(disasm.string);
                if(mo!=null)
                {
                    if(switchReg.indexOf(mo[1])==-1)
                    {
                        print('unk cmp in',curAddr,disasm.string);
                    }
                    else
                    {
                        cmpVal=parseInt(mo[2]);
                        if(isNaN(cmpVal))
                        {
                            if(mo[2]!=' ebx')
                                throw 'cvt cmp digit error! '+curAddr;
                            else
                            {
                                popBranch();
                                continue;
                            }
                        }
                        state='cmp';
                    }
                }
                else if((mo=this._instPattern.addPat.exec(disasm.string))!=null)
                {
                    if(switchReg.indexOf(mo[1])==-1)
                        print("unk add in ",curAddr,disasm.string);
                    else
                    {
                        valDist+=(parseInt(mo[2])|0);
                        state='prepareJmp';
                    }
                }
				else if((mo=this._instPattern.subPat.exec(disasm.string))!=null)
				{
					if(switchReg.indexOf(mo[1])==-1)
						print('unk sub in ',curAddr);
					else 
					{
						valDist-=(parseInt(mo[2])|0);
						state='prepareJmp';
					}
				}
				else if((mo=this._instPattern.jxxPat.exec(disasm.string))!=null)
				{
					var dst=parseInt(mo[2]);
					if(mo[1]=='jmp' && (dst==outAddr || dst==defaultAddr))
					{
						popBranch();
						continue;
					}
				}
                else
                {
                    throw "no recognized inst in init! "+curAddr;
                }
            }
            else if(state=='cmp')
            {
                var mo=this._instPattern.jxxPat.exec(disasm.string);
                if(mo!=null)
                {
                    var symbol=this._getJxxType(mo[1]);
                    var destAddr=parseInt(mo[2]);
                    if(!this.isRegion(destAddr))
                        throw "out region in jxx ,"+curAddr;
                    
                    if(destAddr==defaultAddr)
                    {
                    }
                    else if(destAddr==outAddr)
                        throw 'go to out in cmp?! '+curAddr;
                    else
                    {
                        if(symbol=='=')
                        {
                            if(cmpVal==null || switchTable[cmpVal]!=undefined)
                                throw cmpVal+' exists! in '+curAddr;
                            switchTable[cmpVal]=destAddr;
                        }
                        else
                        {
                            branchTable.push(destAddr);
                        }
                    }
                        
                }
                else if((mo=this._instPattern.cmpPat.exec(disasm.string))!=null)
                {
                    popBranch();
                    branchTable.push(curAddr);
                    processed=false;
                }
                else
                {
                    state='init';
					processed=false;
                }
            }
            else if(state=='prepareJmp')
            {
                var mo=this._instPattern.cmpPat.exec(disasm.string);
                if(mo!=null)
                {
                    if(switchReg.indexOf(mo[1])==-1)
                    {
                        print('unk precmp in',curAddr,disasm.string);
                    }
                    else
                    {
                        var val=parseInt(mo[2]);
                        if(isNaN(val))
                        {
                            throw 'cvt cmp digit error! '+curAddr;
                        }
                        buffVal.push(val);
                    }
                }
                else if((mo=this._instPattern.movzxPat.exec(disasm.string))!=null)
                {
                    var cnt=buffVal.pop();
                    jmpTabel1=mread(parseInt(mo[1]),cnt);
                }
                else if((mo=this._instPattern.jmpTabelPat.exec(disasm.string))!=null)
                {
                    if(jmpTabel1==null)
                        throw 'empty jmptabel1 in prepareJmp'+curAddr;
                        
                    var table2Addr=parseInt(mo[1]);
                    
                    var minTabel={};
                    
                    for(var i=0;i<jmpTabel1.length;i++)
                    {
                        var idx=jmpTabel1.charCodeAt(i);
                        if(minTabel[idx]!=undefined)
                            continue;
                        
                        var dAddr=u32(table2Addr+idx*4);
                        if(dAddr==defaultAddr)
                            continue;
                        if(switchTable[i-valDist]!=undefined)
                            throw (i-valDist)+' exists! in '+curAddr;
                        
                        switchTable[i-valDist]=dAddr;
                    }
                    
                    popBranch();
					continue;
                }
				else if((mo=this._instPattern.jxxPat.exec(disasm.string))!=null)
				{
                    var symbol=this._getJxxType(mo[1]);
                    var destAddr=parseInt(mo[2]);
                    if(!this.isRegion(destAddr))
                        throw "out region in jxx ,"+curAddr;
                    
                    if(destAddr==defaultAddr)
                    {
                    }
					else
					{
						if(symbol=='=')
						{
							if(switchTable[-valDist]!=undefined)
								throw (-valDist)+' exists!';
							switchTable[-valDist]=destAddr;
							state='init';
						}
						else if(symbol!='>')
						{
							throw "unexpected Jxx";
						}
					}
				}
                else
                {
                    throw 'not fit in '+curAddr;
                }
            }
            
            if(processed)
                branchTable[0]+=disasm.length;
        }
        return switchTable;
    },
    
    _getInstType:function(disasm)
    {
        var inst=disasm.string;
        
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
        movzxPat:/movzx\s+\w+, byte ptr \[eax\+([^\]]+)\]/,
        jmpTabelPat:/jmp\s+dword ptr \[([^\+]+)\+\w+\*4\]/,
    },
};

//Analyzer.init(0x1eeddc7,[[0x1eed160,0x1efa393]],0);sw=Analyzer.getSwitch(0x1eeddc7,0x1ef43a9,0x1ef43ab);
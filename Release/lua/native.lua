local modname='native'
local M={}
--_G[modname]=M

local bit = require("bit")

function M.callFunction(addr,callType,regs,...)
  local rcallType=0
  local retType='eax'
  if string.find(callType,'cdecl')~=nil then
    rcallType=0
  elseif string.find(callType,'stdcall')~=nil then
    rcallType=1
  else
    --print(callType);
    error('call type error: '..callType)
  end

  if string.find(callType,'fpu')~=nil then
    retType='fpu'
    rcallType=bit.bor(rcallType,0x10)
  elseif string.find(callType,'xmm')~=nil then
    retType='xmm'
    rcallType=bit.bor(rcallType,0x20)
  end

  local switchTable={
    eax=7,
    ecx=6,
    edx=5,
    ebx=4,
    esi=1,
    edi=0,
  }

  local rregs={}
  for k,v in pairs(regs) do
    if switchTable[k]~=nil then
      rregs[switchTable[k]]=v
    end
  end

  local rargs={...}
  for i=1,#rargs/2 do
    local t=rargs[i]
    rargs[i]=rargs[#rargs+1-i]
    rargs[#rargs+1-i]=t
  end
  local eax,edx,st0,xmm=_CallFunction(addr,rcallType,rregs,rargs)
  local retVal=eax
  if retType=='fpu' then
    retVal=st0
  elseif retType=='xmm' then
    retVal=xmm
  end

  return retVal
end

return M
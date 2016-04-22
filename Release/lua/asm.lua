local M={}

local native=require('native')

function M.makeCdeclFunction(addr,retType)
  return function (...)
    return native.callFunction(addr,'cdecl'..(retType or ''),{},unpack({...}))
  end
end

function M.makeStdcallFunction(addr,retType)
  return function (...)
    return native.callFunction(addr,'stdcall'..(retType or ''),{},unpack({...}))
  end
end

function M.makeThiscallFunction(addr,retType)
  return function (obj,...)
    return native.callFunction(addr,'stdcall'..(retType or ''),{ecx=obj},unpack({...}))
  end
end

return M
var native=require('native');
var mread=native.mread;
var mwrite=native.mwrite;
cloneObject(require('quickfunc'),global);
require('mystring')(global);
var asm=require('asm');
var Hooker=asm.Hooker;
var win32=require('win32');
cloneObject(require('memory'),global);

GetScriptInstanceAddr=0x00467970;

var runned=0;
function mySkill(regs)
{
  if(!runned)
  {
    var obj=u32(regs.esp+4);
    var vt=u32(obj);
    printMem(vt,0x100);
    win32.MessageBoxA(0,'waiting',0,0);
  }
  runned++;
}
function hookSkill(){return Hooker.checkInfo(0x005DE420,function(regs){return mySkill(regs)})};


var scriptInstance=null;
var lua51=lua51 || (function(){
    var o={};
    var mod=win32.GetModuleHandleA('lua5.1.dll');
    o.luaL_loadstring=makeCdeclFunction(win32.GetProcAddress(mod,'luaL_loadstring'));
    o.lua_pcall=makeCdeclFunction(win32.GetProcAddress(mod,'lua_pcall'));
    o.lua_gettop=makeCdeclFunction(win32.GetProcAddress(mod,'lua_gettop'));
    o.lua_pushstring=makeCdeclFunction(win32.GetProcAddress(mod,'lua_pushstring'));
    o.lua_tostring=makeCdeclFunction(win32.GetProcAddress(mod,'lua_tostring'));
    o.lua_pop=makeCdeclFunction(win32.GetProcAddress(mod,'lua_pop'));
    return o;
})();
function luaDo(cmd)
{
  if(!scriptInstance)
  {
    var getInst=makeCdeclFunction(GetScriptInstanceAddr);
    scriptInstance=u32(getInst());
  }
  var ret=lua51.luaL_loadstring(scriptInstance,cmd);
  if(ret)
  {
    print('errl:',ret);
    return;
  }
  ret=lua51.lua_pcall(scriptInstance,0,-1,0);
  if(ret)
  {
    print('erre:',ret);
  }
  var top=lua51.lua_gettop(scriptInstance);
  if(top>0)
  {
    return astr(lua51.lua_tostring(scriptInstance,-1));
  }
  else
  {
    print('top is 0!');
  }
}
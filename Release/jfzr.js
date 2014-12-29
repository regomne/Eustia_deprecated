var native=require('native');
var mread=native.mread;
var mwrite=native.mwrite;
cloneObject(require('quickfunc'),global);
require('mystring')(global);
var asm=require('asm');
var Hooker=asm.Hooker;
var win32=require('win32');
var plugin=require('plugin');
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
var lua51=plugin.getPlugin('lua5.1.dll','cdecl');

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
    print('erre:',astr(lua51.lua_tolstring(scriptInstance,-1,0)));
    return;
  }
  var top=lua51.lua_gettop(scriptInstance);
  if(top>0)
  {
    var s=astr(lua51.lua_tolstring(scriptInstance,-1,0));
    lua51.lua_settop(scriptInstance,-2);
    return s;
  }

}

function luaDoFile(name)
{
  var str=readText(name);
  return luaDo(str);
}

function openConsole()
{
  var k32=plugin.getPlugin('kernel32.dll','stdcall');
  var msvc=plugin.getPlugin('msvcr100.dll','cdecl');
  k32.AllocConsole();
  var stdout=msvc.__iob_func()+0x20;
  msvc.freopen('CONOUT$','w',stdout);
}
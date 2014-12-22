/// @file
/// @brief 插件支持模块

var native=require('native');
var win32=require('win32');
var asm=require('asm');
require('myString')(global);

var exp={};

/// 从搜索路径中加载一个dll文件，其所有导出函数均可被直接通过属性值调用。\n
/// example: \n
/// \code
/// var lua=getPlugin('lua51.dll','cdecl');
/// lua.lua_open(...)
/// \endcode
/// @brief 加载一个dll形式的插件
/// @param dllname dll的文件名
/// @param callType 该dll中的函数的调用约定，若留空，则插件对象将只返回函数地址
/// @return 插件对象

function getPlugin(dllname,callType)
{
  if(callType && (callType!='stdcall' && callType!='cdecl'))
    throw new Error("call type must be stdcall or cdecl!");

  var fname=root.lookupFile(dllname);
  var mod=null;
  if(fname==null)
  {
    mod=win32.GetModuleHandleW((dllname+'\0').encode());
    if(!mod)
      throw new Error("Can't find the dll: "+dllname);
  }
  if(!mod)
  {
    mod=win32.LoadLibraryW((dllname+'\0').encode());
    if(mod==0)
      throw new Error("Can't load dll: "+dllname);
  }

  var func=native.newFunctionWithNamedAccessor(function(prop){
    if(this[prop])
      return this[prop];

    var addr=win32.GetProcAddress(mod,prop);
    if(addr==0)
      throw new Error("Can't find the export function!");

    var val=addr;
    if(callType=='stdcall')
      val=asm.makeStdcallFunction(addr);
    else if(callType=='cdecl')
      val=asm.makeCdeclFunction(addr);

    this[prop]=val;
    return val;
  });

  var plugin=new func();
  var fake=plugin.__this__;
  fake.__modAddress__=mod;
  return plugin;
}

exp.getPlugin=getPlugin;

module.exports=exp;
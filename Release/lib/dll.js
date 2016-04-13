var win32=require('win32');
var plugin=require('plugin');

///用法：require('dll').kernel32.ExitProcess(0);

var dllnames=[
  ['kernel32.dll','stdcall'],
  ['user32.dll','stdcall'],
];

var dll={};
var trimName=name=>name.endsWith('.dll')?name.slice(0,-4):name;
dllnames.forEach(([name,type])=>
  dll[trimName(name)]=plugin.getPlugin(name,type));

module.exports=dll;
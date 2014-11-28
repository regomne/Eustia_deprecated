var CryptoJS=require('CryptoJS');

module.exports=function(ggl)
{

if(ggl.String.prototype.repeat!=undefined)
	return;
Object.defineProperty(ggl.String.prototype,'repeat',{value:function(n)
{
	var a=[];
	while(a.length<n)
	{
		a.push(this);
	}
	return a.join('');
}});

Object.defineProperty(ggl.String.prototype,'format',{value:function(params)
{
	var reg = /\{(\d+)\}/gm;
	return this.replace(reg,function(match,name)
	{
		return params[~~name];
	})
}});

Object.defineProperty(ggl.String.prototype,'ljust',{value:function(n,ch)
{
	if(this.length>=n)
		return this;
	
	if(typeof(ch)!='string' || ch.length!=1)
		ch=' ';
	
	return this+ggl.String.prototype.repeat.call(ch,n-this.length);
}});

Object.defineProperty(ggl.String.prototype,'rjust',{value:function(n,ch)
{
	if(this.length>=n)
		return this;
	
	if(typeof(ch)!='string' || ch.length!=1)
		ch=' ';
	
	return ggl.String.prototype.repeat.call(ch,n-this.length)+this;
}});

Object.defineProperty(ggl.String.prototype,'startswith',{value:function(s)
{
	return (this.slice(0,s.length)==s);
}});

Object.defineProperty(ggl.String.prototype,'decode',{value:function()
{
	var C=CryptoJS;
	return C.enc.Utf16LE.stringify(C.enc.Latin1.parse(this));
}});

Object.defineProperty(ggl.String.prototype,'encode',{value:function()
{
	var C=CryptoJS;
	return C.enc.Latin1.stringify(C.enc.Utf16LE.parse(this));
}});
}
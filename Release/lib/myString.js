var native=require('native');
var CryptoJS=require('CryptoJS');

module.exports=function(ggl)
{
native.setProperty(ggl.String.prototype,'repeat',function(n)
{
	var a=[];
	while(a.length<n)
	{
		a.push(this);
	}
	return a.join('');
},'ed');

native.setProperty(ggl.String.prototype,'format',function(params)
{
	var reg = /\{(\d+)\}/gm;
	return this.replace(reg,function(match,name)
	{
		return params[~~name];
	})
},'de');

native.setProperty(ggl.String.prototype,'ljust',function(n,ch)
{
	if(this.length>=n)
		return this;
	
	if(typeof(ch)!='string' || ch.length!=1)
		ch=' ';
	
	return this+ch.repeat(n-this.length);
},'de');

native.setProperty(ggl.String.prototype,'rjust',function(n,ch)
{
	if(this.length>=n)
		return this;
	
	if(typeof(ch)!='string' || ch.length!=1)
		ch=' ';
	
	return ch.repeat(n-this.length)+this;
},'de');

native.setProperty(ggl.String.prototype,'startswith',function(s)
{
	return (this.substring(0,s.length)==s);
},'de');

native.setProperty(ggl.String.prototype,'decode',function()
{
	var C=CryptoJS;
	return C.enc.Utf16LE.stringify(C.enc.Latin1.parse(this));
},'de');

native.setProperty(ggl.String.prototype,'encode',function()
{
	var C=CryptoJS;
	return C.enc.Latin1.stringify(C.enc.Utf16LE.parse(this));
},'de');
}
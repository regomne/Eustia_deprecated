
setProperty(String.prototype,'repeat',function(n)
{
	var a=[];
	while(a.length<n)
	{
		a.push(this);
	}
	return a.join('');
},'ed');

setProperty(String.prototype,'format',function(params)
{
	var reg = /\{(\d+)\}/gm;
	return this.replace(reg,function(match,name)
	{
		return params[~~name];
	})
},'de');

setProperty(String.prototype,'ljust',function(n,ch)
{
	if(this.length>=n)
		return this;
	
	if(typeof(ch)!='string' || ch.length!=1)
		ch=' ';
	
	return this+ch.repeat(n-this.length);
},'de');

setProperty(String.prototype,'rjust',function(n,ch)
{
	if(this.length>=n)
		return this;
	
	if(typeof(ch)!='string' || ch.length!=1)
		ch=' ';
	
	return ch.repeat(n-this.length)+this;
},'de');

setProperty(String.prototype,'startswith',function(s)
{
	return (this.substring(0,s.length)==s);
},'de');
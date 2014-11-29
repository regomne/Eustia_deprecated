/// @file mystring.js
/// @brief js基本类型String的扩展

/// 扩展指定global对象的String类型，增加原型方法。
/// @brief 扩展字符串
/// @param ggl global对象
/// @function mystring(ggl)

/// 将一个字符串自身重复指定的次数并连接起来，返回连接后的字符串。
/// @brief 字符串重复
/// @param n 重复次数
/// @return 新的字符串
/// @function String.repeat(n)

/// 以自身为模板，格式化一个字符串，返回新的字符串。\n
/// 模板的解析方式与C#类似，形如\n
/// @code {.js}
/// '{0} concat with {1}'.format(['aa','bb'])=='aa concat with bb'
/// @endcode
/// @brief 字符串格式化
/// @param args 格式化参数，以array形式传递
/// @return 新的字符串
/// @function String.format(args)

/// 将字符串左对齐。
/// @brief 将字符串左对齐
/// @param n 对齐后的字符串长度
/// @param ch 填充用的字符，默认为' '
/// @return 新的字符串
/// @function String.ljust(n,ch)

/// 将字符串右对齐。
/// @brief 将字符串右对齐
/// @param n 对齐后的字符串长度
/// @param ch 填充用的字符，默认为' '
/// @return 新的字符串
/// @function String.rjust(n,ch)

/// 判断字符串是否以指定字符串开始。
/// @brief 判断字符串是否以指定字符串开始
/// @param s 判断字符串
/// @return 新的字符串
/// @function String.startswith(s)

/// 将字符串转换成utf16编码，即将每个字符的16位unicode编码中，每8个字节产生一个新的字符组成新的字符串。使用Little Endian方式编码。
/// @brief 将字符串转换成utf16编码
/// @return 新的字符串
/// @function String.encode()

/// 将原始字符串的每两个字符的编码的低8位连在一起组成一个unicode字符编码，并组合成新的字符串。
/// @brief 以utf16编码解析字符串
/// @return 新的字符串
/// @function String.decode()

/// @cond
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

/// @endcond
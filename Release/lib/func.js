/// @file
/// @brief 函数式支持模块

var exp={};

/// 返回各参数函数的与函数
/// @brief 返回各参数函数的与函数
/// @return 返回的与函数
function and()
{
  var args=arguments;
  return function()
  {
    for(var i=0;i<args.length;i++)
    {
      if(!args[i].apply(this,arguments))
        return false;
    }
    return true;
  };
}
exp.and=and;

/// 返回各参数函数的或函数
/// @brief 返回各参数函数的或函数
/// @return 返回的或函数
function or()
{
  var args=arguments;
  return function()
  {
    for(var i=0;i<args.length;i++)
    {
      if(args[i].apply(this,arguments))
        return true;
    }
    return false;
  };
}
exp.or=or;

/// 返回参数函数的非函数
/// @brief 返回参数函数的非函数
/// @param f 待求非的函数
/// @return 非函数
function not(f)
{
  return function()
  {
    var result=f.apply(this,arguments);
    return !result;
  };
}
exp.not=not;

/// 针对数组中的每个元素应用指定函数，并返回新元素按原顺序组成的数组。\n
/// 与ECMAScript5中的Array.prototype.map相同。
/// @brief 将数组中的元素通过f映像为一个新数组
/// @param a 原始数组
/// @param f 映像函数
/// @return 新数组
function map(a,f)
{
  return a.map(f);
}
exp.map=map;

/// 将原始数组按照归纳函数依次归纳成一个值
/// @brief 将原始数组按照归纳函数依次归纳成一个值
/// @param a 原始数组
/// @param f 归纳函数（接受两个参数）
/// @param initial 初始元素（可选）
/// @return 归纳之后的值
function reduce(a,f,initial)
{
  if(arguments.length>2) return a.reduce(f,initial);
  else return a.reduce(f);
}
exp.reduce=reduce;

/// 根据原始单元素函数，返回可对数组作用map函数的函数
/// @brief 根据原始单元素函数，返回可对数组作用map函数的函数
/// @param f 原始的单元素作用函数
/// @return 可对数组作用map函数的函数
function mapper(f)
{
  return function(a)
  {
    return map(a,f);
  };
}
exp.mapper=mapper;

/// 生成一个组合两个函数的函数 根据f和g返回f(g())
/// @brief 生成一个组合两个函数的函数
/// @param f 外层函数
/// @param g 内层函数
/// @return f(g())
function compose(f,g)
{
  return function()
  {
    return f.call(this,g.apply(this,arguments));
  };
}
exp.compose=compose;

/// 将一个类似数组对象转换为数组
/// @brief 将一个类似数组对象转换为数组
/// @param a 类数组对象（要求有[]和length）
/// @param n 从第n个元素开始转换
/// @return 数组对象
function array(a,n)
{
  return Array.prototype.slice.call(a, n||0);
}
exp.array=array;

/// 返回一个不完全函数，将参数绑定的原始函数的左侧
/// @brief 返回一个不完全函数，将参数绑定的原始函数的左侧
/// @param f 原始函数
/// @return 返回绑定参数的新函数
function bindL(f)
{
  var args=arguments;
  return function()
  {
    var a=array(args,1);
    a=a.concat(array(arguments));
    return f.apply(this,a);
  };
}
exp.bindL=bindL;

/// 返回一个不完全函数，将参数绑定的原始函数的右侧
/// @brief 返回一个不完全函数，将参数绑定的原始函数的右侧
/// @param f 原始函数
/// @return 返回绑定参数的新函数
function bindR(f)
{
  var args=arguments;
  return function()
  {
    var a=array(arguments);
    a=a.concat(array(args,1));
    return f.apply(this,a);
  };
}
exp.bindR=bindR;

/// 返回一个不完全函数，将undefined参数进行模板替换，并将剩余参数绑定到右侧
/// @brief 生成函数模板，将其中的undefined代替
/// @param f 原始函数
/// @return 返回绑定参数的新函数
function bind(f)
{
  var args=arguments;
  return function()
  {
    var a=array(args,1);
    var i=0,j=0;
    for(;i<a.length;i++)
    {
      if(a[i]==undefined)
        a[i]=arguments[j++];
    }
    a=a.concat(array(arguments,j));
    return f.apply(this,a);
  }
}
exp.bind=bind;

/// 返回一个带有记忆功能的函数，针对同样的参数(值类型）直接返回相同的结果。\n
/// 主要用在复杂递归场景中，生成的函数有较高的空间复杂度。
/// @param f 用于生成的函数
/// @return 带记忆功能的函数
function memorize(f)
{
  var cache={};
  return function()
  {
    var key=arguments.length+Array.prototype.join.call(arguments,',');
    if(key in cache)
      return cache[key];
    else
      return cache[key]=f.apply(this,arguments);
  };
}
exp.memorize=memorize;

module.exports=exp;
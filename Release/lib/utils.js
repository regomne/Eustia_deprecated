require('mystring')(global);

module.exports.displayObject=function (obj,file)
{
  function display_(obj,level)
  {
    var indentStr='\t'.repeat(level);
    var s=indentStr+'{\r\n';
    indentStr=indentStr+'\t';
    for(var prop in obj)
    {
      s+=indentStr+prop+': ';
      var val=obj[prop];
      if(typeof(val)!='object')
      {
        s+=val.toString(16)+',\r\n';
      }
      // else if(val instanceof Array)
      // {
      //  var s0='[';
      //  for(var i=0;i<val.length;i++)
      //  {
      //    s0+=val[i]
      //  }
      // }
      else
      {
        s+='\r\n';
        s+=display_(val,level+1)+',\r\n';
      }
    }
    s+=indentStr.slice(0,-1)+'}';
    return s;
  }
  var s=display_(obj,0);
  if(file!=undefined)
    writeText(file,s);
  else
    print(s);
}

module.exports.isObjectSame=function (obj1,obj2)
{
  if(obj1===obj2)
    return true;
  for(var att in obj2)
  {
    if(!obj1.hasOwnProperty(att))
      return false;
  }

  for(var att in obj1)
  {
    if(!obj2.hasOwnProperty(att))
      return false;

    var ele1=obj1[att];
    var ele2=obj2[att];
    if(typeof(ele1)!=typeof(ele2))
      return false;

    if(typeof(ele1)!='object')
    {
      if(ele1!==ele2)
        return false;
    }
    else
    {
      if(!isObjectSame(ele1,ele2))
        return false;
    }
  }
  return true;
}


var Convert={
  toU32: function(s,off,be)
  {
    if(off==undefined)
      off=0;
    if(be==undefined)
      be=false;
    if(!be)
    {
      return s.charCodeAt(off)+(s.charCodeAt(off+1)<<8)+
        (s.charCodeAt(off+2)<<16)+(s.charCodeAt(off+3)*(1<<24));
    }
    else
    {
      return s.charCodeAt(off+3)+(s.charCodeAt(off+2)<<8)+
        (s.charCodeAt(off+1)<<16)+(s.charCodeAt(off)*(1<<24));
    }
  },
  toU16: function(s,off,be)
  {
    if(off==undefined)
      off=0;
    if(be==undefined)
      be=false;
    if(!be)
    {
      return s.charCodeAt(off)+(s.charCodeAt(off+1)<<8);
    }
    else
    {
      return s.charCodeAt(off+1)+(s.charCodeAt(off)<<8);
    }
  },
  
  fromU32:function(i,be)
  {
    var s;
    if(be==undefined)
      be=false;
    if(be)
    {
      s=String.fromCharCode((i>>>24)&0xff)+
        String.fromCharCode((i>>>16)&0xff)+
        String.fromCharCode((i>>>8)&0xff)+
        String.fromCharCode(i&0xff);
    }
    else
    {
      s=String.fromCharCode((i)&0xff)+
        String.fromCharCode((i>>>8)&0xff)+
        String.fromCharCode((i>>>16)&0xff)+
        String.fromCharCode((i>>>24)&0xff);
    }
    return s;
  },
  
  fromU16:function(i,be)
  {
    var s;
    if(be==undefined)
      be=false;
    if(be)
    {
      s=String.fromCharCode((i>>>8)&0xff)+
        String.fromCharCode(i&0xff);
    }
    else
    {
      s=String.fromCharCode((i)&0xff)+
        String.fromCharCode((i>>>8)&0xff);
    }
    return s;
  },
  
  parseTbl:
  {
    'I':{func:function(){return Convert.toU32}, cnt:4},
    'H':{func:function(){return Convert.toU16}, cnt:2},
  },
  unpack: function(s,pat)
  {
    var retArr=[];
    var be=false;
    if(pat[0]=='>' || pat[0]=='<')
    {
      if(pat[0]=='>')
        be=true;
      pat=pat.slice(1);
    }
    var i=0;
    for(var c=0;c<pat.length;c++)
    {
      var cvtf=Convert.parseTbl[pat[c]];
      if(cvtf==undefined)
      {
        throw 'unknown escape character!';
      }
      
      if(i+cvtf.cnt>s.length)
        throw 'str too short!';
      retArr.push(cvtf.func()(s,i,be));
      i+=cvtf.cnt;
    }
    return retArr;
  },
  unpackN:function(s,fmt,names)
  {
    if (names==undefined)
    {
      names=[];
    }
    var rslt=[];
    var sp=0;
    for(var i=0;i<fmt.length;i++)
    {
      if(sp>=s.length)
      {
        throw 'str is not long enough!';
      }
      var f=fmt[i];
      switch (f) {
        case 'I':
          rslt.push([names[i],Convert.toU32(s,sp)]);
          sp+=4;
        break;
        case 'H':
          rslt.push([names[i],Convert.toU16(s,sp)]);
          sp+=2;
        break;
        case 'B':
          rslt.push([names[i],s.charCodeAt(sp)]);
          sp+=1;
        break;
        default:
          throw 'unknown escape character: '+f;
      }
    }
    return rslt;
  },

  pack:function(pat)
  {
    var s=[];
    var be=false;
    if(pat[0]=='>' || pat[0]=='<')
    {
      if(pat[0]=='>')
        be=true;
      pat=pat.slice(1);
    }
    for(var i=0;i<pat.length;i++)
    {
      var c=pat[i];
      var v=arguments[i+1];
      if(v==undefined)
        throw 'args not enough';
      switch(c)
      {
        case 'I':
          s.push(Convert.fromU32(v,be));
        break;
        case 'H':
          s.push(Convert.fromU16(v,be));
        break;
        default:
          throw 'unknown escape charactor';
      }
    }
    return s.join('');
  },
  
  swap32: function(d)
  {
    s1=d&0xff;
    s2=(d>>>8)&0xff;
    s3=(d>>>16)&0xff;
    s4=(d>>>24)&0xff;
    return (s1*(1<<24))+(s2<<16)+(s3<<8)+s4;
  }
};

module.exports.Convert=Convert;

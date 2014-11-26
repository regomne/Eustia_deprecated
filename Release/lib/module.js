(function(){

var path={
  dirname: function(str)
  {
    str=str.replace('/','\\');
    var idx=str.lastIndexOf('\\');
    if(idx==-1)
      return '.';
    return str.slice(0,idx+1);
  },
  basename: function(str)
  {
    str=str.replace('/','\\');
    var idx=str.lastIndexOf('\\');
    if(idx==-1)
      return str;
    return str.slice(idx+1);
  },
  join: function()
  {
    var newpath=[];
    for(var i=0;i<arguments.length;i++)
    {
      var p=arguments[i];
      if(i!=arguments.length-1 && p[p.length-1]!='\\')
        p+='\\';
      newpath.push(p);
    }
    return newpath.join('');
  },
};

var debug=function(){
  if(LogLevel==5)
    print.apply(this,arguments);
};

function Module(id, parent)
{
  this.id = id;
  this.exports = {};
  this.parent = parent;
  if (parent && parent.children)
  {
    parent.children.push(this);
  }

  this.filename = null;
  //this.loaded = false;
  this.children = [];
}

Module._cache = {};

Module._getFilename=function(req,par)
{
  req=req.replace('/','\\');
  if(req.slice(-3).toLowerCase()!='.js')
    req+='.js';

  if(req[1]==':' || req[0]=='\\')
    return req;

  if(par)
  {
    if(_ExistsFile(path.join(path.dirname(par.filename),req)))
      return path.join(path.dirname(par.filename),req);
  }

  for(var i=0;i<__Path.length;i++)
  {
    var fname=path.join(__Path[i],req);
    if(_ExistsFile(fname))
      return fname;
  }
  return null;
}

Module._load = function(request, parent, forceReload) {
  if (parent) {
    debug('Module._load REQUEST  ' + (request) + ' parent: ' + parent.id);
  }

  var filename = Module._getFilename(request, parent);
  if(filename==null)
    throw new Error("can't find module: "+request);

  var cacheKey=filename.toLowerCase();
  if(!forceReload)
  {
    var cachedModule = Module._cache[cacheKey];
    if (cachedModule) {
      return cachedModule.exports;
    }
  }

  var module = new Module(filename, parent);

  try {
    module.load(filename);
    Module._cache[cacheKey] = module;
  } catch(e) {
    throw e;
  }

  return module.exports;
};

Module.prototype.load = function(filename) {
  debug('load ' + JSON.stringify(filename) +
        ' for module ' + JSON.stringify(this.id));

  this.filename = filename;

  loadJSModule(this, filename);
};

Module.prototype.require = function(path,forceReload) {
  //assert(typeof path === 'string', 'path must be a string');
  //assert(path, 'missing path');
  return Module._load(path, this, forceReload);
};

// Returns exception if any
Module.prototype._compile = function(content, filename) {
  var self = this;
  // remove shebang
  content = content.replace(/^\#\!.*/, '');

  function require(path,forceReload) {
    return self.require(path,forceReload);
  }

  // require.resolve = function(request) {
  //   return Module._resolveFilename(request, self);
  // };

  var dirname = path.dirname(filename);

  if (self.id !== '.') {
    debug('load submodule');
    // not root module
    var sandbox = {};
    for (var k in global) {
      sandbox[k] = global[k];
    }
    sandbox.require = require;
    //sandbox.import = require;
    sandbox.exports = self.exports;
    sandbox.__filename = filename;
    sandbox.__dirname = dirname;
    sandbox.module = self;
    sandbox.global = sandbox;
    sandbox.root = global;

    _ImportJS(sandbox, content, filename);
  }
};


function stripBOM(content) {
  // Remove byte order marker. This catches EF BB BF (the UTF-8 BOM)
  // because the buffer-to-string conversion in `fs.readFileSync()`
  // translates it to FEFF, the UTF-16 BOM.
  if (content.charCodeAt(0) === 0xFEFF) {
    content = content.slice(1);
  }
  return content;
}

function loadJSModule(module, filename) {
  var content = _ReadText(filename);
  module._compile(content, filename);
};

var rootModule=new Module('.',null);
rootModule.filename=_DllPath+'!root';

this.module=rootModule;

this.require=function(name,forceReload)
{
  return Module._load(name,rootModule,forceReload);
}

})();
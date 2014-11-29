var fs = require('fs')

var functionName = /^\s*\/\/\/\s+@function\s+(.*)$/;
// var type = /^(\s*)\/\/\/\s+@param\s+\{(\w*)\}\s+(.+?)(\s+.*)$/;
// var param = /^(\s*)\/\/\/\s+@param\s+(.+?)\s/;
// var resultType = /^(\s*)\/\/\/\s+@return\s+\{(\w+)\}(\s+.*)$/;

function makeFunc(str)
{
	return ['function '+str+' {}']
}

function writeLine(line) {
	process.stdout.write(line + '\n');
}

fs.readFile(process.argv[2], 'utf8', function (err, data) {
	var lines = data.replace(/\r\n/g,'\n').split('\n');
	var comment = /^\s*\/\/\//;
	var directive = /@(\w+)\s+(.*)$/;
	lines.forEach(function(line) {
		if (line.match(comment)) {
			var d = line.match(functionName);
			if (d) {
				var s=makeFunc(d[1]);
				s.forEach(function(ele){writeLine(ele)});
			} else {
				writeLine(line);
			}
		} else {
			writeLine(line);
		}
	});
});

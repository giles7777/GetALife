var shell = require('shelljs');

shell.echo('Cleaning deploy directory');
shell.rm('-f','../website/giles7777.github.io/*.js');
shell.rm('-rf','../website/giles7777.github.io/static');
shell.cp('-r','build/*','../website/giles7777.github.io');

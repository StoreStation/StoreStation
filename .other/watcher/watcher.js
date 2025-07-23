const chokidar = require('chokidar');
const { exec } = require('child_process');

process.chdir("../../.");
chokidar.watch('.', { ignored: ['node_modules'] }).on('change', (path) => {
    if (!path.endsWith('.c') && !path.endsWith('.cpp') && !path.endsWith('.h') && !path.endsWith('.hpp')) return;
    exec('clear', (a, b, c) => {
        console.log(`${b}File ${path} has been changed. Running make...`);
        exec('make', (err, stdout, stderr) => {
            if (err) {
                console.error(`Error running make: ${err}`);
                return;
            }
            console.log(stdout);
            console.error(stderr);
        });
    });
});

console.log('Watching for file changes...');

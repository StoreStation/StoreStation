const http = require('http');
const fs = require('fs');
const path = require('path');

const port = 3000;
const directory = './';

const server = http.createServer((req, res) => {
  const filePath = path.join(directory, req.url === '/' ? 'index.html' : req.url);

  fs.readFile(filePath, (err, data) => {
    if (err) {
      if (err.code === 'ENOENT') {
        res.writeHead(404);
        res.end('File not found');
      } else {
        res.writeHead(500);
        res.end(`Error loading ${filePath}: ${err}`);
      }
    } else {
      res.writeHead(200);
      res.end(data);
    }
  });
});

server.listen(port, () => {
  console.log(`File server running at http://localhost:${port}/`);
  console.log(`EBOOT AT http://localhost:${port}/EBOOT.PBP`);
});
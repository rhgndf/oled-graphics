var express = require('express')
var cp = require('child_process')
var path = require('path')
var app = express()

function execprogram(name, args) {
 return function(req, res) {
  args = args || [];
  args.push(req.params.x);
  var prog = cp.spawn(path.join('./programs',name), args);
  res.on("close", function () {prog.kill();});
  prog.stdout.pipe(res);
  prog.stderr.pipe(res);
 }
}

app.get('/', function (req, res) {
  res.send('Hello World!')
})

app.get('/helloworld', execprogram('helloworld'));
app.get('/poweredby', execprogram('poweredby'));
app.get('/demo', execprogram('demo'));
app.get('/pi/:x', execprogram('pi'));

app.get('/15s7d', execprogram('playvideo.py', ['15s7d.h264']));
app.get('/thomas', execprogram('playvideo.py', ['thomas.h264']));

app.listen(3000, function () {
  console.log('Example app listening on port 3000!')
})

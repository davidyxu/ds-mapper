var express = require('express');
var app = express();
var bodyParser = require('body-parser');
var db = [];

app.get('/', function (req, res) {
  res.sendFile("index.html", {root: __dirname});
});

//app.post('/event', function(req, res) {
//  req.body = "";
//  req.on('data', function(chunk) {
//    req.body += chunk;
//  });
//  req.on('end', function() {
//    try {
//      var json = JSON.parse(req.body);
//      console.log("parsed");
//    res.send('ok');
//    } catch (err) {
//      console.log("failure");
//      console.log(err);
//      console.log(req.body);
//    res.send('BAD!!!!');
//    }
//  });
//});

app.post('/event', bodyParser.json(), function(req, res) {
  db.push(req.body);
  console.log(db);
  res.send('ok');
});

app.post('/register', bodyParser.json(), function(req, res) {
  res.send('ok');
});

app.get('/data', function(req, res) {
  res.send(db);
});

var server = app.listen(9999, function () {

  var host = server.address().address;
  var port = server.address().port;

  console.log('Starting server on %s:%s', host, port);

});

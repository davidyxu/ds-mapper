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
  if (!req.body)
    res.status(400).send("bad event");

  console.log("Received", req.body.data, "http events from,", req.body.dev_ip);

  if (req.body.data) {
    for (var i = 0; i < req.body.data.length; i++) {
      var datum = req.body.data[i];

      var rawHeader = datum.payload.split("\n\n")[0];
      var headerArr = rawHeader.split("\n");
      var firstLine = headerArr.shift().split(" ");

      var headers = {}
      headerArr.forEach(function(header) {
        var split_header = header.split(": ");
        headers[split_header[0]] = split_header[1];
      });

      var httpEvent = {
        origin: req.body.dev_ip,
        src: datum.src_ip + ":" + datum.src_port,
        dst: datum.dst_ip + ":" + datum.dst_port,
        timestamp: datum.timestamp
      }

      if (datum.http_method) {
        httpEvent.method = datum.http_method;
        httpEvent.path = firstLine[1];
        httpEvent.version = firstLine[2];
      }

      if (datum.http_code) {
        httpEvent.code = datum.code;
      }

      httpEvent.headers = headers;

      db.push(httpEvent);
    }
  }

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

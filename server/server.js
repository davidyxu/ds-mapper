var express = require('express');
var app = express();

var bodyParser = require('body-parser');
var qs = require('querystring');

var db = [];
var simple = [];

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
    res.status(400).send("bad data\n");

  if (req.body.data) {
    var agent_ip = req.body.dev_ip;
    var services = req.body.services;

    console.log(new Date().toISOString(), ":: Received", req.body.data.length, "http events from", agent_ip);
    console.log("Services:", services);

    for (var i = 0; i < req.body.data.length; i++) {
      var datum = req.body.data[i];

      var rawHeader = datum.payload.split("\n\n")[0];
      var headerArr = rawHeader.split("\n");
      var firstLine = headerArr.shift().split(" ");

      var httpEvent = {
        origin: req.body.dev_ip,
        src: datum.src_ip + ":" + datum.src_port,
        dst: datum.dst_ip + ":" + datum.dst_port,
        timestamp: datum.timestamp
      }

      var simpleEvent = {
        source: datum.src_ip,
        target: datum.dst_ip,
        type: datum.http_method || datum.http_code
      }
      for (key in req.body) {
        if (datum.src_ip == agent_ip && services.hasOwnProperty(datum.src_port))
          simpleEvent.source = services[datum.src_port];
        else if (datum.dst_ip == agent_ip && services.hasOwnProperty(datum.dst_port))
          simpleEvent.target = services[datum.dst_port];
      }

      if (datum.http_method) {
        httpEvent.method = datum.http_method;
        if (firstLine.length) {
          var uri = firstLine[1].split("?");
          httpEvent.path = uri[0];

          if (uri[1])
            httpEvent.query = qs.parse(uri[1]);

          httpEvent.version = firstLine[2];
        }

      }

      if (datum.http_code)
        httpEvent.code = datum.http_code;

      if (headerArr.length) {
        var headers = {}
        headerArr.forEach(function(header) {
          var split_header = header.split(": ");
          headers[split_header[0]] = split_header[1];
        });

        httpEvent.headers = headers;
      }

      db.push(httpEvent);
      simple.push(simpleEvent);
    }
  }

  res.send('ok\n');
});

app.post('/register', bodyParser.json(), function(req, res) {
  res.send('ok');
});

app.get('/data', function(req, res) {
  res.send(simple);
});

var server = app.listen(9999, function () {

  var host = server.address().address;
  var port = server.address().port;

  console.log('Starting server on %s:%s', host, port);

});

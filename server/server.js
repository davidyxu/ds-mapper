var express = require('express');
var app = express();

var bodyParser = require('body-parser');

var EventCollection = require('./lib/event_collection')({
  db_url: 'mongodb://localhost:27017/db'
});

app.get('/', function (req, res) {
  res.sendFile("index.html", {root: __dirname});
});

app.post('/event', bodyParser.json({ limit: "5mb" }), function(req, res) {
  if (!req.body) {
    res.status(400).send("bad data\n");
  } else {
    EventCollection.process(req.body.dev_ip, req.body.services, req.body.data);
    res.send('ok\n');
  }
});

app.get('/req/events', function(req, res) {
  EventCollection.find("reqEventCollection", req.query, function(err, docs) {
    console.log(err);
    res.send(docs);
  });
});

app.get('/req/batches', function(req, res) {
  EventCollection.find("reqBatchCollection", req.query, function(err, docs) {
    console.log(err);
    res.send(docs);
  });
});

app.get('/res/events', function(req, res) {
  EventCollection.find("resEventCollection", req.query, function(err, docs) {
    console.log(err);
    res.send(docs);
  });
});

app.get('/res/batches', function(req, res) {
  EventCollection.find("resBatchCollection", req.query, function(err, docs) {
    console.log(err);
    res.send(docs);
  });
});

EventCollection.init(function() {
  var server = app.listen(9999, function () {

    var host = server.address().address;
    var port = server.address().port;

    console.log('Starting server on %s:%s', host, port);
  });
});

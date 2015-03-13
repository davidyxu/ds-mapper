var express = require('express');
var app = express();

var bodyParser = require('body-parser');

var EventCollection = require('./event_collection')({
  db_url: 'mongodb://localhost:27017/db'
});

app.get('/', function (req, res) {
  res.sendFile("index.html", {root: __dirname});
});

app.post('/event', bodyParser.json(), function(req, res) {
  if (!req.body) {
    res.status(400).send("bad data\n");
  } else {
    EventCollection.process(req.body.dev_ip, req.body.serivces, req.body.data);
    res.send('ok\n');
  }
});

app.get('/data', function(req, res) {
  EventCollection.find({}, function(err, docs) {
    console.log(err);
    console.log(docs);

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

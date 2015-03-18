var MongoClient = require('mongodb').MongoClient;
var qs = require('querystring');

var HTTP_METHODS = ["GET", "PUT", "POST", "HEAD", "TRACE", "DELETE", "CONNECT", "OPTIONS"];
var db_url;

function EventCollection() {};

EventCollection.prototype.find = function(options, callback) {
  options = options || {};
  return this.collection.find(options).toArray(callback);
};

EventCollection.prototype.process = function(agent_ip, services, events) {
  console.log(new Date().toISOString(), ":: Received", events.length, "http events from", agent_ip);
  console.log("Services:", services);

  var processedEvents = [];
  for (var i = 0; i < events.length; i++) {
    var rawHeader = events[i].payload.split("\n\n")[0];
    var headerArr = rawHeader.split("\n");
    var firstLine = headerArr.shift().split(" ");

    var method = null;
    var code = null;

    var path = null;
    var query = null;
    var version = null;

    if (HTTP_METHODS.indexOf(firstLine[0]) === -1) {
      version = firstLine[0];
      code = firstLine[1];
    } else {
      method = firstLine[0];
      uri = firstLine[1].split("?");

      path = uri[0];
      if (uri[1]) query = uri[1];
    }

    var headers = {};
    headerArr.forEach(function(header) {
      var split_header = header.split(": ");
      headers[split_header[0].replace(/\./g, '_')] = split_header[1];
    });

    var httpEvent = {
      origin: agent_ip,
      timestamp: events[i].timestamp,

      src_ip: events[i].src_ip,
      src_port: events[i].src_port,

      dst_ip: events[i].dst_ip,
      dst_port: events[i].dst_port,

      source: events[i].src_ip,
      target: events[i].dst_ip,

      method: method,
      code: code,

      path: path,
      query: query,
      headers: headers
    };

    // replace source/target with appropriate service
    for (var key in services) {
      if (events[i].src_ip === agent_ip && services[key] === events[i].src_port)
        httpEvent.source = key;
      else if (events[i].dst_ip === agent_ip && services[key] === events[i].dst_port)
        httpEvent.target = key;
    }

    processedEvents.push(httpEvent);
  }

  this.collection.insert(processedEvents, function(err, result) {
    if (err) console.log(err);
    if (result) console.log(result);
  });
};

EventCollection.prototype.close = function() {
  if (this.db) {
    this.db.close();
    this.db = null;
  };
};

EventCollection.prototype.init = function(callback) {
  MongoClient.connect(db_url, function(err, db) {
    if (err) throw err;

    this.db = db;
    this.collection = db.collection('events');

    console.log("connected");

    if (callback) callback();
  }.bind(this));
};

module.exports = function(options) {
  db_url = options.db_url;

  return new EventCollection;
};

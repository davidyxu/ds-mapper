var MongoClient = require('mongodb').MongoClient;
var qs = require('querystring');
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

    var path = null;
    var query = null;
    var version = null;

    if (events[i].http_method && firstLine.length) {
      var uri = firstLine[1].split("?");

      path = uri[0];
      if (uri[1]) query = qs.parse(uri[1]);
    } else if (events[i].http_code) {
      version = firstLine[0];
    }

    var headers = {};
    headerArr.forEach(function(header) {
      var split_header = header.split(": ");
      headers[split_header[0]] = split_header[1];
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

      method: events[i].http_method,
      code: events[i].http_code,

      path: path,
      query: query,
      headers: headers
    };

    // replace source/target with appropriate service
    for (var key in services) {
      if (events[i].src_ip === agent_ip && services.hasOwnProperty(events[i].src_port))
        httpEvent.source = services[events[i].src_port];
      else if (events[i].dst_ip === agent_ip && services.hasOwnProperty(events[i].dst_port))
        httpEvent.target = services[events[i].dst_port];
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

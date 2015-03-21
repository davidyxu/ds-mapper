var MongoClient = require('mongodb').MongoClient;
var qs = require('querystring');

var HTTP_METHODS = ["GET", "PUT", "POST", "HEAD", "TRACE", "DELETE", "CONNECT", "OPTIONS"];
var db_url;

function EventCollection() {};

EventCollection.prototype.find = function(options, callback) {
  options = options || {};
  return this.eventCollection.find(options).toArray(callback);
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

  this.eventCollection.insert(processedEvents, function(err, result) {
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

/*
  {
    "33_33_33_10": {
      "33_33_33_1:80": {
        total_requests: X,
        "/": 3,
        "/test": 4
      }
    }
  }
*/

EventCollection.prototype.reduce = function(events) {
  var reqBatch = {};
  var resBatch = {};
  var isRequest = false;

  events.forEach(function(e) {
    var batch, src, dst;
    if (e.method)
      isRequest = true;
    if (isRequest) {
      batch = reqBatch;
      src = e.src_ip;
      dst = e.dst_ip + ":" + e.dst_port;
    else { // response
      batch = resBatch;
      src = e.src_ip + ":" + e.src_ip;
      dst = e.dst_ip;
    }

    if (!batch[src])
      batch[src] = {};
    if (!batch[src][dst])
      batch[src][dst] = { total_requests = 0, paths = {} };

    ++batch[src][dst].total_requests;

    if (!batch[src][dst].paths[e.path])
      batch[src][dst].paths[e.path] = 1;
    else
      ++batch[src][dst].paths[e.path];

    if (isRequest) {
      batch[src][dst].methods = {};

      if (!batch[src][dst].methods[e.method])
        batch[src][dst].methods[e.method] = 1;
      else
        ++reqBatch[src][dst].methods[e.method];
    } else {
      batch[src][dst].codes = {};

      if (!batch[src][dst].codes[e.code])
        batch[src][dst].codes[e.code] = 1;
      else
        ++reqBatch[src][dst].codes[e.code];
    }
  });

  this.reqBatchCollection.insert(reqBatch);
  this.resBatchCollection.insert(resBatch);
};

EventCollection.prototype.init = function(callback) {
  MongoClient.connect(db_url, function(err, db) {
    if (err) throw err;

    this.db = db;
    this.eventCollection = db.collection('events');

    this.reqEventCollection = db.collection('request_event');
    this.resEventCollection = db.collection('response_event');

    this.reqBatchCollection = db.collection('request_batch');
    this.resBatchCollection = db.collection('response_batch');

    console.log("connected");

    if (callback) callback();
  }.bind(this));
};

module.exports = function(options) {
  db_url = options.db_url;

  return new EventCollection;
};

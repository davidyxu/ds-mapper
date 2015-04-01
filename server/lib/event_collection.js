var MongoClient = require("mongodb").MongoClient;
var utils = require("./utils");
var qs = require("querystring");

var db_url;

function EventCollection() {};

EventCollection.prototype.find = function(collection, options, callback) {
  options = options || {};
  return this[collection].find(options).toArray(callback);
};

EventCollection.prototype.close = function() {
  if (this.db) {
    this.db.close();
    this.db = null;
  };
};

EventCollection.prototype.process = function(agentIP, services, events) {
  var req = { events: [], batch: {} };
  var res = { events: [], batch: {} };

  events.forEach(function(e) {
    var group, src, dst;
    var payload = utils.parsePayload(e.payload);

    if (payload.isRequest)
      group = req;
    else
      group = res;

    if (e.src_ip === agentIP)
      src = e.src_ip + ":" + e.dst_port;
    else
      src = e.src_ip;

    if (e.dst_ip === agentIP)
      dst = e.dst_ip + ':' + e.dst_port;
    else
      dst = e.dst_ip;

    for (var key in services) {
      if (e.src_ip === agentIP && services[key] === e.src_port)
        src = key;
      else if (e.dst_ip === agentIP && services[key] === e.dst_port)
        dst = key;
    }

    src = utils.mongoSanitize(src);
    dst = utils.mongoSanitize(dst);

    group.events.push({
      origin: agentIP,
      timestamp: e.timestamp,
      source: src,
      destination: dst,
      src_ip: e.src_ip,
      src_port: e.src_port,
      dst_ip: e.dst_ip,
      dst_port: e.dst_port,
      method: payload.method,
      code: payload.code,
      path: payload.path,
      query: payload.query,
      headers: payload.headers
    });

    if (!group.batch[src])
      group.batch[src] = {};
    if (!group.batch[src][dst])
      group.batch[src][dst] = { total_requests: 0 };

    ++group.batch[src][dst].total_requests;

    if (payload.isRequest) {
      if (!group.batch[src][dst].paths)
        group.batch[src][dst].paths = {};

      var sanitizedPath = utils.mongoSanitize(payload.path);
      if (!group.batch[src][dst].paths[sanitizedPath])
        group.batch[src][dst].paths[sanitizedPath] = 1;
      else
        ++group.batch[src][dst].paths[sanitizedPath];

      if (!group.batch[src][dst].methods)
        group.batch[src][dst].methods = {};

      if (!group.batch[src][dst].methods[payload.method])
        group.batch[src][dst].methods[payload.method] = 1;
      else
        ++group.batch[src][dst].methods[payload.method];
    } else {
      if (!group.batch[src][dst].codes)
        group.batch[src][dst].codes = {};

      if (!group.batch[src][dst].codes[payload.code])
        group.batch[src][dst].codes[payload.code] = 1;
      else
        ++group.batch[src][dst].codes[payload.code];
    }
  });

  // temp debugging lines
  console.log("REQUESTS");
  console.log(JSON.stringify(req.batch));
  console.log("RESPONSES");
  console.log(JSON.stringify(res.batch));
  this.reqEventCollection.insert(req.events, function(err, result) {
    if (err) console.log(err);
  });

  this.resEventCollection.insert(res.events, function(err, result) {
    if (err) console.log(err);
  });

  this.reqBatchCollection.insert(req.batch, function(err, result) {
    if (err) console.log(err);
  });

  this.resBatchCollection.insert(res.batch, function(err, result) {
    if (err) console.log(err);
  });
};

EventCollection.prototype.init = function(callback) {
  MongoClient.connect(db_url, function(err, db) {
    if (err) throw err;
    this.db = db;

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

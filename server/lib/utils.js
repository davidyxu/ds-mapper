var HTTP_METHODS = ["GET", "PUT", "POST", "HEAD", "TRACE", "DELETE", "CONNECT", "OPTIONS"];
function httpMethods() {
  return HTTP_METHODS;
};

function unixTime() {
  return Math.floor(Date.now() / 1000);
};

// mongoDB does not accept '.' in keys
function mongoSanitize(key) {
  return key ? key.replace(/\./g, '_') : "";
};

// parses http payload and returns information of interest
// version: string, http protocol version
// isRequest: bool, true if payload is a request
// method: string, http method
// code: int, http response code
// path: string, uri path
// query: string, uri query
// header: object, http header
function parsePayload(rawPayload) {
  var payload = {}

  var rawHeader = rawPayload.split("\n\n")[0];
  var headerArr = rawHeader.split("\n");
  var firstLine = headerArr.shift().split(" ");

  if (HTTP_METHODS.indexOf(firstLine[0]) === -1) {
    payload.isRequest = false;
    payload.version = firstLine[0];
    payload.code = parseInt(firstLine[1]);
  } else {
    payload.isRequest = true;
    payload.method = firstLine[0];

    var uri = firstLine[1].split("?");

    payload.path = uri[0];
    if (uri[1])
      payload.query = uri[1];
  }

  payload.headers = {};
  headerArr.forEach(function(header) {
    var splitHeader = header.split(": ");
    payload.headers[mongoSanitize(splitHeader[0])] = splitHeader[1];
  });

  return payload;
}


module.exports = {
  httpMethods: httpMethods,
  mongoSanitize: mongoSanitize,
  parsePayload: parsePayload
};

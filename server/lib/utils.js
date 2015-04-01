var HTTP_METHODS = ["GET", "PUT", "POST", "HEAD", "TRACE", "DELETE", "CONNECT", "OPTIONS"];

function httpMethods() {
  return HTTP_METHODS;
};

// mongoDB does not accept '.' in keys
function mongoSanitize(key) {
  return key ? key.replace(/\./g, '_') : "";
};

function parsePayload(rawPayload) {
  var payload = {}

  var rawHeader = rawPayload.split("\n\n")[0];
  var headerArr = rawHeader.split("\n");
  var firstLine = headerArr.shift().split(" ");

  if (HTTP_METHODS.indexOf(firstLine[0]) === -1) {
    payload.isRequest = false;
    payload.version = firstLine[0];
    payload.code = firstLine[1];
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

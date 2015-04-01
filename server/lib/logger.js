function logFactory(context) {
  return function() {
    var args = Array.apply(null, arguments);
    args = [new Date().toISOString(), '[' + context + ']'].concat(args);

    console.log.apply(null, args);
  };
};

module.exports = logFactory;

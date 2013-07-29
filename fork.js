var bindings = require("bindings")("fork.node");

module.exports = {
  forkAndReopenStdio: function(stdin_filename, stdout_filename) {
    var res = bindings.forkAndReopenStdio(stdin_filename, stdout_filename);
    if (res === 0) {
      process.pid = bindings.getpid()
    }
    return res;
  }
}

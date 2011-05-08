/* Escape & Query building tests */

var drizzle = require("./db-drizzle");
var tests = require("./lib/node-db/tests.js").get(function() {
    return new drizzle.Database();
});

for(var test in tests) {
    exports[test] = tests[test];
}

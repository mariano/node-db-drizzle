/* Escape & Query building tests */

require("./db-drizzle");
var tests = require("./lib/node-db/tests.js").get(function() {
    return new Drizzle();
});

for(var test in tests) {
    exports[test] = tests[test];
}

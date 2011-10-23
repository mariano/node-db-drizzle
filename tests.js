/* Escape & Query building tests */

var settings = JSON.parse(require('fs').readFileSync('./tests-settings.json','utf8'));

var drizzle = require("./db-drizzle");
var tests = require("./lib/node-db/tests.js").get(function(callback) {
    new drizzle.Database(settings).connect(function(err) {
        if (err) {
            throw new Error('Could not connect to test DB');
        }
        callback(this);
    });
});

for(var test in tests) {
    exports[test] = tests[test];
}

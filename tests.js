var assert = require("assert");
var node_drizzle = require("./drizzle_bindings");

var drizzle = new node_drizzle.Drizzle();

assert.equal("test", drizzle.escape("test"));
assert.equal("test \\\"string\\\"", drizzle.escape("test \"string\""));
assert.equal("test \\'string\\'", drizzle.escape("test \'string\'"));

drizzle.query("SELECT * FROM users", {
    start: function(query) {
        assert.equal("SELECT * FROM users", query);
        return false;
    }
});

assert.throws(
    function() {
        drizzle.query("SELECT * FROM users WHERE id = ?");
    },
    /Argument .* mandatory/
);

assert.throws(
    function() {
        drizzle.query("SELECT * FROM users WHERE id = ?", {});
    },
    /Wrong number of values/
);

assert.throws(
    function() {
        drizzle.query("SELECT * FROM users WHERE id = ?", [], {});
    },
    /Wrong number of values/
);

drizzle.query("SELECT * FROM users WHERE id = ?", 
    [ 2 ],
    {
        start: function(query) {
            assert.equal("SELECT * FROM users WHERE id = 2", query);
            return false;
        }
    }
);

drizzle.query("SELECT * FROM users WHERE id = ? AND created > ?", 
    [ 2, "2011-03-09 12:00:00" ],
    {
        start: function(query) {
            assert.equal("SELECT * FROM users WHERE id = 2 AND created > '2011-03-09 12:00:00'", query);
            return false;
        }
    }
);

drizzle.query("SELECT * FROM users WHERE id IN ?", 
    [ [1, 2] ],
    {
        start: function(query) {
            assert.equal("SELECT * FROM users WHERE id IN (1,2)", query);
            return false;
        }
    }
);

drizzle.query("SELECT * FROM users WHERE role IN ?", 
    [ ["admin", "moderator"] ],
    {
        start: function(query) {
            assert.equal("SELECT * FROM users WHERE role IN ('admin','moderator')", query);
            return false;
        }
    }
);

drizzle.query("SELECT * FROM users WHERE name IN ?", 
    [ ["John Doe", "Jane O'Hara"] ],
    {
        start: function(query) {
            assert.equal("SELECT * FROM users WHERE name IN ('John Doe','Jane O\\'Hara')", query);
            return false;
        }
    }
);

drizzle.query("SELECT *, 'Use ? mark' FROM users WHERE id = ? AND created > ?", 
    [ 2, "2011-03-09 12:00:00" ],
    {
        start: function(query) {
            assert.equal("SELECT *, 'Use ? mark' FROM users WHERE id = 2 AND created > '2011-03-09 12:00:00'", query);
            return false;
        }
    }
);

drizzle.query("SELECT *, 'Use ? mark', ? FROM users WHERE id = ? AND created > ?", 
    [ "Escape 'quotes' for safety", 2, "2011-03-09 12:00:00" ],
    {
        start: function(query) {
            assert.equal("SELECT *, 'Use ? mark', 'Escape \\'quotes\\' for safety' FROM users WHERE id = 2 AND created > '2011-03-09 12:00:00'", query);
            return false;
        }
    }
);

drizzle.query("SELECT *, 'Use ? mark', Unquoted\\?mark, ? FROM users WHERE id = ? AND created > ?", 
    [ "Escape 'quotes' for safety", 2, "2011-03-09 12:00:00" ],
    {
        start: function(query) {
            assert.equal("SELECT *, 'Use ? mark', Unquoted?mark, 'Escape \\'quotes\\' for safety' FROM users WHERE id = 2 AND created > '2011-03-09 12:00:00'", query);
            return false;
        }
    }
);

drizzle.query("\\?SELECT *, 'Use ? mark', Unquoted\\?mark, ? FROM users WHERE id = ? AND created > ?", 
    [ "Escape 'quotes' for safety", 2, "2011-03-09 12:00:00" ],
    {
        start: function(query) {
            assert.equal("?SELECT *, 'Use ? mark', Unquoted?mark, 'Escape \\'quotes\\' for safety' FROM users WHERE id = 2 AND created > '2011-03-09 12:00:00'", query);
            return false;
        }
    }
);

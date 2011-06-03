# db-drizzle: Drizzle database bindings for Node.js #

For detailed information about this and other Node.js
database bindings visit the [Node.js DB homepage] [homepage].

## INSTALL ##

```bash
$ npm install db-drizzle
```

## QUICK START ##

```javascript
var drizzle = require('db-drizzle');
new drizzle.Database({
    hostname: 'localhost',
    user: 'root',
    password: 'password',
    database: 'node'
}).connect(function(error) {
    if (error) {
        return console.log("CONNECTION ERROR: " + error);
    }

    this.query().select('*').from('users').execute(function(error, rows) {
        if (error) {
            return console.log('ERROR: ' + error);
        }
        console.log(rows.length + ' ROWS');
    });
});
```

## LICENSE ##

This module is released under the [MIT License] [license].

[homepage]: http://nodejsdb.org
[license]: http://www.opensource.org/licenses/mit-license.php

/*!
 * Copyright by Mariano Iglesias
 *
 * See license text in LICENSE file
 */

/**
 * Require bindings native binary
 *
 * @ignore
 */
var EventEmitter = require('events').EventEmitter,
    binding;

try {
    binding = require("./build/default/drizzle_bindings");
} catch(error) {
    binding = require("./build/Release/drizzle_bindings");
}

function extend(target, source) {
    for (var k in source.prototype) {
        target.prototype[k] = source.prototype[k];
    }
    return target;
}

exports.Query = extend(binding.Query, EventEmitter);
exports.Database = extend(binding.Drizzle, EventEmitter);

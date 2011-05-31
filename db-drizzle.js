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
var binding = require("./build/default/drizzle_bindings");
exports.Database = binding.Drizzle;
exports.Query = binding.Query;

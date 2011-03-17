// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./node-db/binding.h"
#include "./drizzle.h"
#include "./query.h"

extern "C" {
    void init(v8::Handle<v8::Object> target) {
        node_db_drizzle::Drizzle::Init(target);
        node_db_drizzle::Query::Init(target);
    }

    NODE_MODULE(drizzle_bindings, init);
}

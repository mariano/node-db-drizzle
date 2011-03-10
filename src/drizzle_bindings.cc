// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./drizzle_bindings.h"
#include "./drizzle.h"
#include "./query.h"

extern "C" {
    void init(v8::Handle<v8::Object> target) {
        node_drizzle::Drizzle::Init(target);
        node_drizzle::Query::Init(target);
    }

    NODE_MODULE(drizzle_bindings, init);
}

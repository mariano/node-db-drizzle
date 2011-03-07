#include "drizzle_bindings.h"
#include "drizzle.h"

using namespace node_drizzle;

extern "C" {
    void init(Handle<Object> target) {
        Drizzle::Init(target);
    }

    NODE_MODULE(drizzle_bindings, init);
}

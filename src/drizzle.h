// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#ifndef SRC_DRIZZLE_H_
#define SRC_DRIZZLE_H_

#include <node.h>
#include <node_events.h>
#include <string>
#include "./drizzle/connection.h"
#include "./drizzle/exception.h"
#include "./drizzle_bindings.h"
#include "./query.h"

namespace node_drizzle {
class Drizzle : public node::EventEmitter {
    public:
        drizzle::Connection connection;

        static void Init(v8::Handle<v8::Object> target);

    protected:
        struct connect_request_t {
            Drizzle* drizzle;
            const char* error;
        };
        static v8::Persistent<v8::String> syReady;
        static v8::Persistent<v8::String> syError;

        Drizzle();
        ~Drizzle();
        static v8::Handle<v8::Value> New(const v8::Arguments& args);
        static v8::Handle<v8::Value> Connect(const v8::Arguments& args);
        static v8::Handle<v8::Value> Disconnect(const v8::Arguments& args);
        static v8::Handle<v8::Value> IsConnected(const v8::Arguments& args);
        static v8::Handle<v8::Value> Escape(const v8::Arguments& args);
        static v8::Handle<v8::Value> Table(const v8::Arguments& args);
        static v8::Handle<v8::Value> Field(const v8::Arguments& args);
        static v8::Handle<v8::Value> Query(const v8::Arguments& args);
        static int eioConnect(eio_req* req);
        static void connect(connect_request_t* request);
        static void connectFinished(connect_request_t* request);
        static int eioConnectFinished(eio_req* eioRequest);
        v8::Handle<v8::Value> set(const v8::Arguments& args);
};
}

#endif  // SRC_DRIZZLE_H_

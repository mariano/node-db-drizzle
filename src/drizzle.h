// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#ifndef SRC_DRIZZLE_H_
#define SRC_DRIZZLE_H_

#include <stdlib.h>
#include <node.h>
#include <node_buffer.h>
#include <node_events.h>
#include <string>
#include <sstream>
#include <vector>
#include "./drizzle/connection.h"
#include "./drizzle/exception.h"
#include "./drizzle/result.h"
#include "./drizzle_bindings.h"

namespace node_drizzle {
class Drizzle : public node::EventEmitter {
    public:
        static void Init(v8::Handle<v8::Object> target);

    protected:
        typedef enum {
            COLUMN_TYPE_STRING,
            COLUMN_TYPE_BOOL,
            COLUMN_TYPE_INT,
            COLUMN_TYPE_NUMBER,
            COLUMN_TYPE_DATE,
            COLUMN_TYPE_TIME,
            COLUMN_TYPE_DATETIME,
            COLUMN_TYPE_TEXT,
            COLUMN_TYPE_SET
        } column_type_t;
        struct connect_request_t {
            Drizzle* drizzle;
            const char* error;
            v8::Persistent<v8::Function> cbSuccess;
            v8::Persistent<v8::Function> cbError;
        };
        struct query_request_t {
            bool cast;
            bool buffer;
            Drizzle* drizzle;
            std::string query;
            drizzle::Result *result;
            const char* error;
            std::vector<std::string**>* rows;
            v8::Persistent<v8::Function> cbStart;
            v8::Persistent<v8::Function> cbFinish;
            v8::Persistent<v8::Function> cbSuccess;
            v8::Persistent<v8::Function> cbError;
            v8::Persistent<v8::Function> cbEach;
        };
        drizzle::Connection *connection;

        Drizzle();
        ~Drizzle();
        static v8::Handle<v8::Value> New(const v8::Arguments& args);
        static v8::Handle<v8::Value> Connect(const v8::Arguments& args);
        static v8::Handle<v8::Value> Disconnect(const v8::Arguments& args);
        static v8::Handle<v8::Value> Escape(const v8::Arguments& args);
        static v8::Handle<v8::Value> Query(const v8::Arguments& args);
        static int eioConnect(eio_req* req);
        static void connect(connect_request_t* request);
        static void connectFinished(connect_request_t* request);
        static int eioConnectFinished(eio_req* eioRequest);
        static int eioQuery(eio_req* eioRequest);
        static int eioQueryFinished(eio_req* eioRequest);
        static int eioQueryEach(eio_req* eioRequest);
        static int eioQueryEachFinished(eio_req* eioRequest);
        static void eioQueryCleanup(query_request_t* request);
        static void eioQueryRequestFree(query_request_t* request);
        v8::Local<v8::Object> row(drizzle::Result* result, std::string** currentRow, bool cast) const;
        std::string parseQuery(const std::string& query, v8::Local<v8::Array> values) const throw(drizzle::Exception&);
        std::string value(v8::Local<v8::Value> value, bool inArray = false) const throw(drizzle::Exception&);

    private:
        uint64_t toDate(const std::string& value, bool hasTime) const throw(drizzle::Exception&);
        std::string fromDate(const uint64_t timeStamp) const throw(drizzle::Exception&);
        uint64_t toTime(const std::string& value) const;
        // GMT delta calculation borrowed from https://github.com/Sannis/node-mysql-libmysqlclient
        int gmtDelta() const throw(drizzle::Exception&);
};
}

#endif  // SRC_DRIZZLE_H_

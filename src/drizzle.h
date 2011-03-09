#ifndef _NODE_DRIZZLE__DRIZZLE_H
#define _NODE_DRIZZLE__DRIZZLE_H

#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>
#include <node.h>
#include <node_buffer.h>
#include <node_events.h>
#include "drizzle/connection.h"
#include "drizzle/exception.h"
#include "drizzle/result.h"
#include "drizzle_bindings.h"

using namespace v8;

namespace node_drizzle {
class Drizzle : public node::EventEmitter {
    public:
        static void Init(Handle<Object> target);

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
            Persistent<Function> cbSuccess;
            Persistent<Function> cbError;
        };
        struct query_request_t {
            bool cast;
            bool buffer;
            Drizzle* drizzle;
            std::string query;
            drizzle::Result *result;
            const char* error;
            std::vector<std::string**>* rows;
            Persistent<Function> cbStart;
            Persistent<Function> cbFinish;
            Persistent<Function> cbSuccess;
            Persistent<Function> cbError;
            Persistent<Function> cbEach;
        };
        drizzle::Connection *connection;

        Drizzle();
        ~Drizzle();
        static Handle<Value> New(const Arguments& args);
        static Handle<Value> Connect(const Arguments& args);
        static Handle<Value> Disconnect(const Arguments& args);
        static Handle<Value> Escape(const Arguments& args);
        static Handle<Value> Query(const Arguments& args);
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
        Local<Object> row(drizzle::Result* result, std::string** currentRow, bool cast) const;
        std::string parseQuery(const std::string& query, Local<Array> values) const throw(drizzle::Exception&);
        std::string value(Local<Value> value) const throw(drizzle::Exception&);

    private:
        uint64_t toDate(const std::string& value, bool hasTime) const throw(drizzle::Exception&);
        std::string fromDate(const uint64_t timeStamp) const throw(drizzle::Exception&);
        uint64_t toTime(const std::string& value) const;
        // GMT delta calculation borrowed from https://github.com/Sannis/node-mysql-libmysqlclient
        int gmtDelta() const throw(drizzle::Exception&);
};
}

#endif

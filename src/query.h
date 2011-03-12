// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#ifndef SRC_QUERY_H_
#define SRC_QUERY_H_

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
class Query : public node::EventEmitter {
    public:
        static v8::Persistent<v8::FunctionTemplate> constructorTemplate;

        static void Init(v8::Handle<v8::Object> target);
        void setConnection(drizzle::Connection* connection);
        v8::Handle<v8::Value> set(const v8::Arguments& args);

    protected:
        struct execute_request_t {
            Query* query;
            drizzle::Result *result;
            const char* error;
            std::vector<std::string**>* rows;
        };
        drizzle::Connection* connection;
        std::ostringstream sql;
        bool cast;
        v8::Persistent<v8::Array> values;
        v8::Persistent<v8::Function>* cbStart;
        v8::Persistent<v8::Function>* cbSuccess;
        v8::Persistent<v8::Function>* cbFinish;
        static v8::Persistent<v8::String> syError;
        static v8::Persistent<v8::String> sySuccess;
        static v8::Persistent<v8::String> syEach;
        static const char quoteString;
        static const char quoteField;
        static const char quoteTable;

        Query();
        ~Query();
        static v8::Handle<v8::Value> New(const v8::Arguments& args);
        static v8::Handle<v8::Value> Select(const v8::Arguments& args);
        static v8::Handle<v8::Value> From(const v8::Arguments& args);
        static v8::Handle<v8::Value> Execute(const v8::Arguments& args);
        static int eioExecute(eio_req* eioRequest);
        static int eioExecuteFinished(eio_req* eioRequest);
        std::string selectField(v8::Local<v8::Value> value) const throw(drizzle::Exception&);
        v8::Local<v8::Object> row(drizzle::Result* result, std::string** currentRow, bool cast) const;
        std::string parseQuery(const std::string& query, v8::Persistent<v8::Array> values) const throw(drizzle::Exception&);
        std::string value(v8::Local<v8::Value> value, bool inArray = false, bool escape = true) const throw(drizzle::Exception&);

    private:
        uint64_t toDate(const std::string& value, bool hasTime) const throw(drizzle::Exception&);
        std::string fromDate(const uint64_t timeStamp) const throw(drizzle::Exception&);
        uint64_t toTime(const std::string& value) const;
        // GMT delta calculation borrowed from https://github.com/Sannis/node-mysql-libmysqlclient
        int gmtDelta() const throw(drizzle::Exception&);
};
}

#endif  // SRC_QUERY_H_

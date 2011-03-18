// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./drizzle.h"

v8::Persistent<v8::FunctionTemplate> node_db_drizzle::Drizzle::constructorTemplate;

node_db_drizzle::Drizzle::Drizzle(): node_db::Binding() {
    this->connection = new node_db_drizzle::Connection();
    assert(this->connection);
}

node_db_drizzle::Drizzle::~Drizzle() {
    if (this->connection != NULL) {
        delete this->connection;
    }
}

void node_db_drizzle::Drizzle::Init(v8::Handle<v8::Object> target) {
    v8::HandleScope scope;

    v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);

    constructorTemplate = v8::Persistent<v8::FunctionTemplate>::New(t);
    constructorTemplate->Inherit(node::EventEmitter::constructor_template);
    constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

    node_db::Binding::Init(target, constructorTemplate);

    target->Set(v8::String::NewSymbol("Drizzle"), constructorTemplate->GetFunction());
}

v8::Handle<v8::Value> node_db_drizzle::Drizzle::New(const v8::Arguments& args) {
    v8::HandleScope scope;

    node_db_drizzle::Drizzle* binding = new node_db_drizzle::Drizzle();
    if (binding == NULL) {
        THROW_EXCEPTION("Can't create client object")
    }

    if (args.Length() > 0) {
        v8::Handle<v8::Value> set = binding->set(args);
        if (!set.IsEmpty()) {
            return scope.Close(set);
        }
    }

    binding->Wrap(args.This());

    return scope.Close(args.This());
}

v8::Handle<v8::Value> node_db_drizzle::Drizzle::set(const v8::Arguments& args) {
    v8::Handle<v8::Value> result = node_db::Binding::set(args);

    v8::Local<v8::Object> options = args[0]->ToObject();
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_BOOL(options, mysql);

    node_db_drizzle::Connection* connection = static_cast<node_db_drizzle::Connection*>(this->connection);

    if (options->Has(mysql_key)) {
        connection->setMysql(options->Get(mysql_key)->IsTrue());
    }

    return result;
}

v8::Persistent<v8::Object> node_db_drizzle::Drizzle::createQuery() const {
    v8::Persistent<v8::Object> query(
        node_db_drizzle::Query::constructorTemplate->GetFunction()->NewInstance());
    return query;
}

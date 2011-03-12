// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./drizzle.h"

v8::Persistent<v8::String> node_drizzle::Drizzle::syReady;
v8::Persistent<v8::String> node_drizzle::Drizzle::syError;

void node_drizzle::Drizzle::Init(v8::Handle<v8::Object> target) {
    v8::HandleScope scope;

    v8::Local<v8::FunctionTemplate> functionTemplate = v8::FunctionTemplate::New(New);
    functionTemplate->Inherit(node::EventEmitter::constructor_template);

    v8::Local<v8::ObjectTemplate> instanceTemplate = functionTemplate->InstanceTemplate();
    instanceTemplate->SetInternalFieldCount(1);

    NODE_ADD_CONSTANT(instanceTemplate, COLUMN_TYPE_STRING, drizzle::Result::Column::STRING);
    NODE_ADD_CONSTANT(instanceTemplate, COLUMN_TYPE_BOOL, drizzle::Result::Column::BOOL);
    NODE_ADD_CONSTANT(instanceTemplate, COLUMN_TYPE_INT, drizzle::Result::Column::INT);
    NODE_ADD_CONSTANT(instanceTemplate, COLUMN_TYPE_NUMBER, drizzle::Result::Column::NUMBER);
    NODE_ADD_CONSTANT(instanceTemplate, COLUMN_TYPE_DATE, drizzle::Result::Column::DATE);
    NODE_ADD_CONSTANT(instanceTemplate, COLUMN_TYPE_TIME, drizzle::Result::Column::TIME);
    NODE_ADD_CONSTANT(instanceTemplate, COLUMN_TYPE_DATETIME, drizzle::Result::Column::DATETIME);
    NODE_ADD_CONSTANT(instanceTemplate, COLUMN_TYPE_TEXT, drizzle::Result::Column::TEXT);
    NODE_ADD_CONSTANT(instanceTemplate, COLUMN_TYPE_SET, drizzle::Result::Column::SET);

    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "connect", Connect);
    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "disconnect", Disconnect);
    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "isConnected", IsConnected);
    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "escape", Escape);
    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "table", Table);
    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "field", Field);
    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "query", Query);

    syReady = NODE_PERSISTENT_SYMBOL("ready");
    syError = NODE_PERSISTENT_SYMBOL("error");

    target->Set(v8::String::NewSymbol("Drizzle"), functionTemplate->GetFunction());
}

node_drizzle::Drizzle::Drizzle(): node::EventEmitter() {
}

node_drizzle::Drizzle::~Drizzle() {
}

v8::Handle<v8::Value> node_drizzle::Drizzle::New(const v8::Arguments& args) {
    v8::HandleScope scope;

    node_drizzle::Drizzle *drizzle = new node_drizzle::Drizzle();
    if (drizzle == NULL) {
        THROW_EXCEPTION("Can't create client object")
    }

    if (args.Length() > 0) {
        v8::Handle<v8::Value> set = drizzle->set(args);
        if (!set.IsEmpty()) {
            return set;
        }
    }

    drizzle->Wrap(args.This());

    return args.This();
}

v8::Handle<v8::Value> node_drizzle::Drizzle::Connect(const v8::Arguments& args) {
    v8::HandleScope scope;

    node_drizzle::Drizzle *drizzle = node::ObjectWrap::Unwrap<node_drizzle::Drizzle>(args.This());
    assert(drizzle);

    bool async = true;

    if (args.Length() > 0) {
        v8::Handle<v8::Value> set = drizzle->set(args);
        if (!set.IsEmpty()) {
            return set;
        }

        v8::Local<v8::Object> options = args[0]->ToObject();

        ARG_CHECK_OBJECT_ATTR_OPTIONAL_BOOL(options, async);

        if (options->Has(async_key) && options->Get(async_key)->IsFalse()) {
            async = false;
        }
    }

    connect_request_t *request = new connect_request_t();
    if (request == NULL) {
        THROW_EXCEPTION("Could not create EIO request")
    }

    request->drizzle = drizzle;
    request->error = NULL;

    if (async) {
        request->drizzle->Ref();
        eio_custom(eioConnect, EIO_PRI_DEFAULT, eioConnectFinished, request);
        ev_ref(EV_DEFAULT_UC);
    } else {
        connect(request);
        connectFinished(request);
    }

    return v8::Undefined();
}

v8::Handle<v8::Value> node_drizzle::Drizzle::set(const v8::Arguments& args) {
    ARG_CHECK_OBJECT(0, options);

    v8::Local<v8::Object> options = args[0]->ToObject();

    ARG_CHECK_OBJECT_ATTR_OPTIONAL_STRING(options, hostname);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_STRING(options, user);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_STRING(options, password);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_STRING(options, database);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_UINT32(options, port);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_BOOL(options, mysql);

    v8::String::Utf8Value hostname(options->Get(hostname_key)->ToString());
    v8::String::Utf8Value user(options->Get(user_key)->ToString());
    v8::String::Utf8Value password(options->Get(password_key)->ToString());
    v8::String::Utf8Value database(options->Get(database_key)->ToString());

    if (options->Has(hostname_key)) {
        this->connection.setHostname(*hostname);
    }

    if (options->Has(user_key)) {
        this->connection.setUser(*user);
    }

    if (options->Has(password_key)) {
        this->connection.setPassword(*password);
    }

    if (options->Has(database_key)) {
        this->connection.setDatabase(*database);
    }

    if (options->Has(port_key)) {
        this->connection.setPort(options->Get(mysql_key)->ToInt32()->Value());
    }

    if (options->Has(mysql_key)) {
        this->connection.setMysql(options->Get(mysql_key)->IsTrue());
    }

    return v8::Handle<v8::Value>();
}

void node_drizzle::Drizzle::connect(connect_request_t* request) {
    try {
        request->drizzle->connection.open();
    } catch(const drizzle::Exception& exception) {
        request->error = exception.what();
    }
}

void node_drizzle::Drizzle::connectFinished(connect_request_t* request) {
    bool connected = request->drizzle->connection.isOpened();

    if (connected) {
        v8::Local<v8::Object> server = v8::Object::New();
        server->Set(v8::String::New("version"), v8::String::New(request->drizzle->connection.version().c_str()));
        server->Set(v8::String::New("hostname"), v8::String::New(request->drizzle->connection.getHostname().c_str()));
        server->Set(v8::String::New("user"), v8::String::New(request->drizzle->connection.getUser().c_str()));
        server->Set(v8::String::New("database"), v8::String::New(request->drizzle->connection.getDatabase().c_str()));

        v8::Local<v8::Value> argv[1];
        argv[0] = server;

        request->drizzle->Emit(syReady, 1, argv);
    } else {
        v8::Local<v8::Value> argv[1];
        argv[0] = v8::String::New(request->error != NULL ? request->error : "(unknown error)");

        request->drizzle->Emit(syError, 1, argv);
    }

    delete request;
}

int node_drizzle::Drizzle::eioConnect(eio_req* eioRequest) {
    connect_request_t *request = static_cast<connect_request_t *>(eioRequest->data);
    assert(request);

    connect(request);

    return 0;
}

int node_drizzle::Drizzle::eioConnectFinished(eio_req* eioRequest) {
    v8::HandleScope scope;

    connect_request_t *request = static_cast<connect_request_t *>(eioRequest->data);
    assert(request);

    ev_unref(EV_DEFAULT_UC);
    request->drizzle->Unref();

    connectFinished(request);

    return 0;
}

v8::Handle<v8::Value> node_drizzle::Drizzle::Disconnect(const v8::Arguments& args) {
    v8::HandleScope scope;

    node_drizzle::Drizzle *drizzle = node::ObjectWrap::Unwrap<node_drizzle::Drizzle>(args.This());
    assert(drizzle);

    drizzle->connection.close();

    return v8::Undefined();
}

v8::Handle<v8::Value> node_drizzle::Drizzle::IsConnected(const v8::Arguments& args) {
    v8::HandleScope scope;

    node_drizzle::Drizzle *drizzle = node::ObjectWrap::Unwrap<node_drizzle::Drizzle>(args.This());
    assert(drizzle);

    return scope.Close(drizzle->connection.isOpened() ? v8::True() : v8::False());
}

v8::Handle<v8::Value> node_drizzle::Drizzle::Escape(const v8::Arguments& args) {
    v8::HandleScope scope;

    ARG_CHECK_STRING(0, string);

    node_drizzle::Drizzle *drizzle = node::ObjectWrap::Unwrap<node_drizzle::Drizzle>(args.This());
    assert(drizzle);

    std::string escaped;

    try {
        v8::String::Utf8Value string(args[0]->ToString());
        std::string unescaped(*string);
        escaped = drizzle->connection.escape(unescaped);
    } catch(const drizzle::Exception& exception) {
        THROW_EXCEPTION(exception.what())
    }

    return scope.Close(v8::String::New(escaped.c_str()));
}

v8::Handle<v8::Value> node_drizzle::Drizzle::Table(const v8::Arguments& args) {
    v8::HandleScope scope;

    ARG_CHECK_STRING(0, table);

    node_drizzle::Drizzle *drizzle = node::ObjectWrap::Unwrap<node_drizzle::Drizzle>(args.This());
    assert(drizzle);

    std::ostringstream escaped;

    try {
        v8::String::Utf8Value string(args[0]->ToString());
        std::string unescaped(*string);
        escaped << node_drizzle::Query::quoteTable << unescaped << node_drizzle::Query::quoteTable;
    } catch(const drizzle::Exception& exception) {
        THROW_EXCEPTION(exception.what())
    }

    return scope.Close(v8::String::New(escaped.str().c_str()));
}

v8::Handle<v8::Value> node_drizzle::Drizzle::Field(const v8::Arguments& args) {
    v8::HandleScope scope;

    ARG_CHECK_STRING(0, field);

    node_drizzle::Drizzle *drizzle = node::ObjectWrap::Unwrap<node_drizzle::Drizzle>(args.This());
    assert(drizzle);

    std::ostringstream escaped;

    try {
        v8::String::Utf8Value string(args[0]->ToString());
        std::string unescaped(*string);
        escaped << node_drizzle::Query::quoteField << unescaped << node_drizzle::Query::quoteField;
    } catch(const drizzle::Exception& exception) {
        THROW_EXCEPTION(exception.what())
    }

    return scope.Close(v8::String::New(escaped.str().c_str()));
}

v8::Handle<v8::Value> node_drizzle::Drizzle::Query(const v8::Arguments& args) {
    v8::HandleScope scope;

    node_drizzle::Drizzle *drizzle = node::ObjectWrap::Unwrap<node_drizzle::Drizzle>(args.This());
    assert(drizzle);

    v8::Persistent<v8::Object> query(
        node_drizzle::Query::constructorTemplate->GetFunction()->NewInstance());

    node_drizzle::Query *queryInstance = node::ObjectWrap::Unwrap<node_drizzle::Query>(query);
    queryInstance->setConnection(&drizzle->connection);

    v8::Handle<v8::Value> set = queryInstance->set(args);
    if (!set.IsEmpty()) {
        return set;
    }

    return scope.Close(query);
}

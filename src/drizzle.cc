#include "drizzle.h"

using namespace node_drizzle;

void Drizzle::Init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->Inherit(EventEmitter::constructor_template);
    t->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(t, "connect", Connect);
    NODE_SET_PROTOTYPE_METHOD(t, "disconnect", Disconnect);
    NODE_SET_PROTOTYPE_METHOD(t, "query", Query);

    target->Set(String::NewSymbol("Drizzle"), t->GetFunction());
}

Drizzle::Drizzle(): EventEmitter(),
    connection(NULL)
{
}

Drizzle::~Drizzle() {
    if (this->connection != NULL) {
        delete this->connection;
    }
}

Handle<Value> Drizzle::New(const Arguments& args) {
    HandleScope scope;

    Drizzle *drizzle = new Drizzle();
    drizzle->Wrap(args.This());

    return args.This();
}

Handle<Value> Drizzle::Connect(const Arguments& args) {
    HandleScope scope;

    ARG_CHECK_OBJECT(0, options);

    Local<Object> options = args[0]->ToObject();

    ARG_CHECK_OBJECT_ATTR_STRING(options, hostname);
    ARG_CHECK_OBJECT_ATTR_STRING(options, user);
    ARG_CHECK_OBJECT_ATTR_STRING(options, password);
    ARG_CHECK_OBJECT_ATTR_STRING(options, database);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_UINT32(options, port);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_BOOL(options, mysql);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, success);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, error);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_BOOL(options, async);

    Drizzle *drizzle = ObjectWrap::Unwrap<Drizzle>(args.This());
    assert(drizzle);

    if (drizzle->connection != NULL) {
        delete drizzle->connection;
    }

    String::Utf8Value hostname(options->Get(hostname_key)->ToString());
    String::Utf8Value user(options->Get(user_key)->ToString());
    String::Utf8Value password(options->Get(password_key)->ToString());
    String::Utf8Value database(options->Get(database_key)->ToString());

    drizzle->connection = new drizzle::Connection();
    drizzle->connection->setHostname(*hostname);
    drizzle->connection->setUser(*user);
    drizzle->connection->setPassword(*password);
    drizzle->connection->setDatabase(*database);

    if (options->Has(port_key)) {
        drizzle->connection->setPort(options->Get(mysql_key)->ToInt32()->Value());
    }

    if (options->Has(mysql_key)) {
        drizzle->connection->setMysql(options->Get(mysql_key)->IsTrue());
    }

    connect_request_t *request = new connect_request_t();
    request->drizzle = drizzle;
    request->error = NULL;

    if (options->Has(success_key)) {
        request->cbSuccess = Persistent<Function>::New(Local<Function>::Cast(options->Get(success_key)));
    }

    if (options->Has(error_key)) {
        request->cbError = Persistent<Function>::New(Local<Function>::Cast(options->Get(error_key)));
    }

    bool async = options->Has(async_key) ? options->Get(async_key)->IsTrue() : true;
    if (async) {
        request->drizzle->Ref();
        eio_custom(eioConnect, EIO_PRI_DEFAULT, eioConnectFinished, request);
        ev_ref(EV_DEFAULT_UC);
    } else {
        connect(request);
        connectFinished(request);
    }

    return Undefined();
}

void Drizzle::connect(connect_request_t* request) {
    try {
        request->drizzle->connection->open();
    } catch(drizzle::Exception& exception) {
        request->error = exception.what();
    }
}

void Drizzle::connectFinished(connect_request_t* request) {
    bool connected = request->drizzle->connection->isOpened();

    TryCatch tryCatch;

    if (connected) {
        Local<Object> server = Object::New();
        server->Set(String::New("version"), String::New(request->drizzle->connection->version().c_str()));
        server->Set(String::New("hostname"), String::New(request->drizzle->connection->getHostname().c_str()));
        server->Set(String::New("user"), String::New(request->drizzle->connection->getUser().c_str()));
        server->Set(String::New("database"), String::New(request->drizzle->connection->getDatabase().c_str()));

        Local<Value> argv[1];
        argv[0] = server;

        request->cbSuccess->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        Local<Value> argv[1];
        argv[0] = String::New(request->error != NULL ? request->error : "(unknown error)");
        request->cbError->Call(Context::GetCurrent()->Global(), 1, argv);
    }

    if (tryCatch.HasCaught()) {
        node::FatalException(tryCatch);
    }

    request->cbSuccess.Dispose();
    request->cbError.Dispose();

    delete request;
}

int Drizzle::eioConnect(eio_req* eioRequest) {
    connect_request_t *request = static_cast<connect_request_t *>(eioRequest->data);
    assert(request);

    connect(request);

    return 0;
}

int Drizzle::eioConnectFinished(eio_req* eioRequest) {
    HandleScope scope;

    connect_request_t *request = static_cast<connect_request_t *>(eioRequest->data);
    assert(request);

    ev_unref(EV_DEFAULT_UC);
    request->drizzle->Unref();

    connectFinished(request);

    return 0;
}

Handle<Value> Drizzle::Disconnect(const Arguments& args) {
    HandleScope scope;

    Drizzle *drizzle = ObjectWrap::Unwrap<Drizzle>(args.This());
    assert(drizzle);

    if (drizzle->connection != NULL) {
        drizzle->connection->close();
        delete drizzle->connection;
    }

    return Undefined();
}

Handle<Value> Drizzle::Query(const Arguments& args) {
    HandleScope scope;

    ARG_CHECK_STRING(0, query);
    ARG_CHECK_OBJECT(1, options);

    Local<Object> options = args[1]->ToObject();

    ARG_CHECK_OBJECT_ATTR_OPTIONAL_BOOL(options, buffer);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, success);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, error);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, each);

    String::Utf8Value query(args[0]->ToString());

    Drizzle *drizzle = ObjectWrap::Unwrap<Drizzle>(args.This());
    assert(drizzle);

    query_request_t *request = new query_request_t();
    request->drizzle = drizzle;
    request->buffer = true;
    request->runEach = false;
    request->query = *query;
    request->result = NULL;
    request->rows = NULL;
    request->error = NULL;

    if (options->Has(buffer_key)) {
        request->buffer = options->Get(buffer_key)->IsTrue();
    }

    if (options->Has(success_key)) {
        request->cbSuccess = Persistent<Function>::New(Local<Function>::Cast(options->Get(success_key)));
    }

    if (options->Has(error_key)) {
        request->cbError = Persistent<Function>::New(Local<Function>::Cast(options->Get(error_key)));
    }

    if (options->Has(each_key)) {
        request->runEach = true;
        request->cbEach = Persistent<Function>::New(Local<Function>::Cast(options->Get(each_key)));
    }

    request->drizzle->Ref();
    eio_custom(eioQuery, EIO_PRI_DEFAULT, eioQueryFinished, request);
    ev_ref(EV_DEFAULT_UC);

    return Undefined();
}

int Drizzle::eioQuery(eio_req* eioRequest) {
    query_request_t *request = static_cast<query_request_t *>(eioRequest->data);
    assert(request);

    try {
        request->result = request->drizzle->connection->query(request->query);

        if (request->buffer) {
            request->rows = new std::vector<std::string**>();
            if (request->rows == NULL) {
                throw drizzle::Exception("Could not create buffer for rows");
            }

            uint16_t columnCount = request->result->columnCount();
            while (request->result->hasNext()) {
                const char **currentRow = request->result->next();
                std::string** row = new std::string*[columnCount];
                if (row == NULL) {
                    throw drizzle::Exception("Could not create buffer for row");
                }

                for (uint16_t i=0; i < columnCount; i++) {
                    if (currentRow[i] != NULL) {
                        row[i] = new std::string(currentRow[i]);
                    } else {
                        row[i] = NULL;
                    }
                }

                request->rows->push_back(row);
            }
        }
    } catch(drizzle::Exception& exception) {
        if (request->rows != NULL) {
            delete request->rows;
        }
        request->error = exception.what();
    }

    return 0;
}

int Drizzle::eioQueryFinished(eio_req* eioRequest) {
    HandleScope scope;

    query_request_t *request = static_cast<query_request_t *>(eioRequest->data);
    assert(request);

    uint16_t columnCount = (request->result != NULL ? request->result->columnCount() : 0);
    if (request->error == NULL) {
        assert(request->result);

        Local<Value> argv[2];
        uint8_t argc = 1;

        Local<Array> columns = Array::New(columnCount);
        for (uint16_t j=0; j < columnCount; j++) {
            drizzle::Result::Column *currentColumn = request->result->column(j);
            Local<Object> column = Object::New();
            column->Set(String::New("name"), String::New(currentColumn->getName().c_str()));
            columns->Set(j, column);
        }

        argv[0] = columns;

        Local<Array> rows = Array::New();

        if (request->buffer) {
            assert(request->rows);

            uint64_t index=0;
            for (std::vector<std::string**>::iterator iterator = request->rows->begin(); iterator != request->rows->end(); ++iterator, index++) {
                std::string** row = *iterator;
                rows->Set(index, request->drizzle->row(request->result, row));
            }

            argc = 2;
            argv[1] = rows;
        }

        TryCatch tryCatch;
        request->cbSuccess->Call(Context::GetCurrent()->Global(), argc, argv);
        if (tryCatch.HasCaught()) {
            node::FatalException(tryCatch);
        }

        if (request->buffer && request->runEach) {
            uint64_t index=0;
            for (std::vector<std::string**>::iterator iterator = request->rows->begin(); iterator != request->rows->end(); ++iterator, index++) {
                Local<Value> eachArgv[3];

                eachArgv[0] = rows->Get(index);
                eachArgv[1] = Number::New(index);
                eachArgv[2] = *(iterator + 1 == request->rows->end() ? True() : False());

                TryCatch tryCatch;
                request->cbEach->Call(Context::GetCurrent()->Global(), 3, eachArgv);
                if (tryCatch.HasCaught()) {
                    node::FatalException(tryCatch);
                }
            }
        }
    } else {
        Local<Value> argv[1];
        argv[0] = String::New(request->error != NULL ? request->error : "(unknown error)");

        TryCatch tryCatch;
        request->cbError->Call(Context::GetCurrent()->Global(), 1, argv);
        if (tryCatch.HasCaught()) {
            node::FatalException(tryCatch);
        }
    }

    eioQueryCleanup(request);

    return 0;
}

int Drizzle::eioQueryEach(eio_req* eioRequest) {
    query_request_t *request = static_cast<query_request_t *>(eioRequest->data);
    assert(request);

    try {
        if (request->result->hasNext()) {
            const char **currentRow = request->result->next();
            request->rows = new std::vector<std::string**>();
            if (request->rows == NULL) {
                throw drizzle::Exception("Could not create buffer for rows");
            }

            uint16_t columnCount = request->result->columnCount();
            std::string** row = new std::string*[columnCount];
            if (row == NULL) {
                throw drizzle::Exception("Could not create buffer for row");
            }

            for (uint16_t i=0; i < columnCount; i++) {
                if (currentRow[i] != NULL) {
                    row[i] = new std::string(currentRow[i]);
                } else {
                    row[i] = NULL;
                }
            }

            request->rows->push_back(row);
        }
    } catch(drizzle::Exception& exception) {
        if (request->rows != NULL) {
            delete request->rows;
        }
        request->error = exception.what();
    }

    return 0;
}

int Drizzle::eioQueryEachFinished(eio_req* eioRequest) {
    HandleScope scope;

    query_request_t *request = static_cast<query_request_t *>(eioRequest->data);
    assert(request);

    if (request->rows != NULL) {
        Local<Value> eachArgv[3];

        eachArgv[0] = request->drizzle->row(request->result, request->rows->front());
        eachArgv[1] = Number::New(request->result->index());
        eachArgv[2] = *(request->result->hasNext() ? False() : True());

        TryCatch tryCatch;
        request->cbEach->Call(Context::GetCurrent()->Global(), 3, eachArgv);
        if (tryCatch.HasCaught()) {
            node::FatalException(tryCatch);
        }
    }

    eioQueryCleanup(request);

    return 0;
}

void Drizzle::eioQueryCleanup(query_request_t* request) {
    ev_unref(EV_DEFAULT_UC);
    request->drizzle->Unref();

    if (request->rows != NULL) {
        uint16_t columnCount = request->result->columnCount();
        for (std::vector<std::string**>::iterator iterator = request->rows->begin(); iterator != request->rows->end(); ++iterator) {
            std::string** row = *iterator;
            for (uint16_t i=0; i < columnCount; i++) {
                if (row[i] != NULL) {
                    delete row[i];
                }
            }
            delete [] row;
        }

        delete request->rows;
    }

    if (request->result == NULL || !request->result->hasNext()) {
        if (request->result != NULL) {
            delete request->result;
        }

        request->cbSuccess.Dispose();
        request->cbError.Dispose();
        request->cbEach.Dispose();

        delete request;
    } else {
        request->drizzle->Ref();
        eio_custom(eioQueryEach, EIO_PRI_DEFAULT, eioQueryEachFinished, request);
        ev_ref(EV_DEFAULT_UC);
    }
}

Local<Object> Drizzle::row(drizzle::Result* result, std::string** currentRow) {
    Local<Object> row = Object::New();

    for (uint16_t j=0, limitj = result->columnCount(); j < limitj; j++) {
        Local<String> columnName = String::New(result->column(j)->getName().c_str());
        if (currentRow[j] != NULL) {
            row->Set(columnName, String::New(currentRow[j]->c_str()));
        } else {
            row->Set(columnName, Null());
        }
    }

    return row;
}


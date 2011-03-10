// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./drizzle.h"

v8::Persistent<v8::String> node_drizzle::Drizzle::sySuccess;
v8::Persistent<v8::String> node_drizzle::Drizzle::syError;

void node_drizzle::Drizzle::Init(v8::Handle<v8::Object> target) {
    v8::HandleScope scope;

    v8::Local<v8::FunctionTemplate> functionTemplate = v8::FunctionTemplate::New(New);
    functionTemplate->Inherit(node::EventEmitter::constructor_template);

    v8::Local<v8::ObjectTemplate> instanceTemplate = functionTemplate->InstanceTemplate();
    instanceTemplate->SetInternalFieldCount(1);

    NODE_DEFINE_CONSTANT(instanceTemplate, COLUMN_TYPE_STRING);
    NODE_DEFINE_CONSTANT(instanceTemplate, COLUMN_TYPE_BOOL);
    NODE_DEFINE_CONSTANT(instanceTemplate, COLUMN_TYPE_INT);
    NODE_DEFINE_CONSTANT(instanceTemplate, COLUMN_TYPE_NUMBER);
    NODE_DEFINE_CONSTANT(instanceTemplate, COLUMN_TYPE_DATE);
    NODE_DEFINE_CONSTANT(instanceTemplate, COLUMN_TYPE_TIME);
    NODE_DEFINE_CONSTANT(instanceTemplate, COLUMN_TYPE_DATETIME);
    NODE_DEFINE_CONSTANT(instanceTemplate, COLUMN_TYPE_TEXT);
    NODE_DEFINE_CONSTANT(instanceTemplate, COLUMN_TYPE_SET);

    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "connect", Connect);
    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "disconnect", Disconnect);
    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "isConnected", IsConnected);
    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "escape", Escape);
    NODE_ADD_PROTOTYPE_METHOD(functionTemplate, "query", Query);

    sySuccess = NODE_PERSISTENT_SYMBOL("success");
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
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Can't create client object")));
    }

    if (args.Length() > 0) {
        v8::Handle<v8::Value> set = drizzle->set(args);
        if (!set->IsUndefined()) {
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
        if (!set->IsUndefined()) {
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
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Could not create EIO request")));
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

    return v8::Undefined();
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

        request->drizzle->Emit(sySuccess, 1, argv);
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
        return v8::ThrowException(v8::Exception::Error(v8::String::New(exception.what())));
    }

    return scope.Close(v8::String::New(escaped.c_str()));
}

v8::Handle<v8::Value> node_drizzle::Drizzle::Query(const v8::Arguments& args) {
    v8::HandleScope scope;

    ARG_CHECK_STRING(0, query);

    if (args.Length() > 2) {
        ARG_CHECK_ARRAY(1, values);
        ARG_CHECK_OBJECT(2, options);
    } else {
        ARG_CHECK_OBJECT(1, options);
    }

    v8::Local<v8::Object> options = args[args.Length() > 2 ? 2 : 1]->ToObject();

    ARG_CHECK_OBJECT_ATTR_OPTIONAL_BOOL(options, buffer);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_BOOL(options, cast);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, start);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, finish);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, success);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, error);
    ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, each);

    v8::String::Utf8Value query(args[0]->ToString());

    node_drizzle::Drizzle *drizzle = node::ObjectWrap::Unwrap<node_drizzle::Drizzle>(args.This());
    assert(drizzle);

    query_request_t *request = new query_request_t();
    if (request == NULL) {
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Could not create EIO request")));
    }

    request->drizzle = drizzle;
    request->cast = true;
    request->buffer = true;
    request->query = *query;
    request->result = NULL;
    request->rows = NULL;
    request->error = NULL;

    if (options->Has(buffer_key)) {
        request->buffer = options->Get(buffer_key)->IsTrue();
    }

    if (options->Has(cast_key)) {
        request->cast = options->Get(cast_key)->IsTrue();
    }

    if (options->Has(start_key)) {
        request->cbStart = v8::Persistent<v8::Function>::New(v8::Local<v8::Function>::Cast(options->Get(start_key)));
    }

    if (options->Has(finish_key)) {
        request->cbFinish = v8::Persistent<v8::Function>::New(v8::Local<v8::Function>::Cast(options->Get(finish_key)));
    }

    if (options->Has(success_key)) {
        request->cbSuccess = v8::Persistent<v8::Function>::New(v8::Local<v8::Function>::Cast(options->Get(success_key)));
    }

    if (options->Has(error_key)) {
        request->cbError = v8::Persistent<v8::Function>::New(v8::Local<v8::Function>::Cast(options->Get(error_key)));
    }

    if (options->Has(each_key)) {
        request->cbEach = v8::Persistent<v8::Function>::New(v8::Local<v8::Function>::Cast(options->Get(each_key)));
    }

    v8::Local<v8::Array> values;

    if (args.Length() > 2) {
        values = v8::Array::Cast(*args[1]);
    } else {
        values = v8::Array::New();
    }

    try {
        request->query = drizzle->parseQuery(request->query, values);
    } catch(const drizzle::Exception& exception) {
        return v8::ThrowException(v8::Exception::Error(v8::String::New(exception.what())));
    }

    if (!request->cbStart.IsEmpty()) {
        v8::Local<v8::Value> argv[1];
        argv[0] = v8::String::New(request->query.c_str());

        v8::TryCatch tryCatch;
        v8::Handle<v8::Value> result = request->cbStart->Call(v8::Context::GetCurrent()->Global(), 1, argv);
        if (tryCatch.HasCaught()) {
            node::FatalException(tryCatch);
        }

        if (!result->IsUndefined()) {
            if (result->IsFalse()) {
                eioQueryRequestFree(request);
                return v8::Undefined();
            } else if (result->IsString()) {
                v8::String::Utf8Value modifiedQuery(result->ToString());
                request->query = *modifiedQuery;
            }
        }
    }

    request->drizzle->Ref();
    eio_custom(eioQuery, EIO_PRI_DEFAULT, eioQueryFinished, request);
    ev_ref(EV_DEFAULT_UC);

    return v8::Undefined();
}

int node_drizzle::Drizzle::eioQuery(eio_req* eioRequest) {
    query_request_t *request = static_cast<query_request_t *>(eioRequest->data);
    assert(request);

    try {
        request->result = request->drizzle->connection.query(request->query);
        if (request->buffer && request->result != NULL) {
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

                for (uint16_t i = 0; i < columnCount; i++) {
                    if (currentRow[i] != NULL) {
                        row[i] = new std::string(currentRow[i]);
                    } else {
                        row[i] = NULL;
                    }
                }

                request->rows->push_back(row);
            }
        }
    } catch(const drizzle::Exception& exception) {
        if (request->rows != NULL) {
            delete request->rows;
        }
        request->error = exception.what();
    }

    return 0;
}

int node_drizzle::Drizzle::eioQueryFinished(eio_req* eioRequest) {
    v8::HandleScope scope;

    query_request_t *request = static_cast<query_request_t *>(eioRequest->data);
    assert(request);

    uint16_t columnCount = (request->result != NULL ? request->result->columnCount() : 0);
    if (request->error == NULL) {
        assert(request->result);

        v8::Local<v8::Value> argv[2];
        uint8_t argc = 1;

        v8::Local<v8::Array> columns = v8::Array::New(columnCount);
        for (uint16_t j = 0; j < columnCount; j++) {
            drizzle::Result::Column *currentColumn = request->result->column(j);
            v8::Local<v8::Value> columnType;

            switch (currentColumn->getType()) {
                case drizzle::Result::Column::BOOL:
                    columnType = NODE_CONSTANT(COLUMN_TYPE_BOOL);
                    break;
                case drizzle::Result::Column::INT:
                    columnType = NODE_CONSTANT(COLUMN_TYPE_INT);
                    break;
                case drizzle::Result::Column::NUMBER:
                    columnType = NODE_CONSTANT(COLUMN_TYPE_NUMBER);
                    break;
                case drizzle::Result::Column::DATE:
                    columnType = NODE_CONSTANT(COLUMN_TYPE_DATE);
                    break;
                case drizzle::Result::Column::TIME:
                    columnType = NODE_CONSTANT(COLUMN_TYPE_TIME);
                    break;
                case drizzle::Result::Column::DATETIME:
                    columnType = NODE_CONSTANT(COLUMN_TYPE_DATETIME);
                    break;
                case drizzle::Result::Column::TEXT:
                    columnType = NODE_CONSTANT(COLUMN_TYPE_TEXT);
                    break;
                case drizzle::Result::Column::SET:
                    columnType = NODE_CONSTANT(COLUMN_TYPE_SET);
                    break;
                default:
                    columnType = NODE_CONSTANT(COLUMN_TYPE_STRING);
                    break;
            }

            v8::Local<v8::Object> column = v8::Object::New();
            column->Set(v8::String::New("name"), v8::String::New(currentColumn->getName().c_str()));
            column->Set(v8::String::New("type"), columnType);

            columns->Set(j, column);
        }

        argv[0] = columns;

        v8::Local<v8::Array> rows = v8::Array::New();

        if (request->buffer) {
            assert(request->rows);

            uint64_t index = 0;
            for (std::vector<std::string**>::iterator iterator = request->rows->begin(), end = request->rows->end(); iterator != end; ++iterator, index++) {
                std::string** row = *iterator;
                rows->Set(index, request->drizzle->row(request->result, row, request->cast));
            }

            argc = 2;
            argv[1] = rows;
        }

        if (!request->cbSuccess.IsEmpty()) {
            v8::TryCatch tryCatch;
            request->cbSuccess->Call(v8::Context::GetCurrent()->Global(), argc, argv);
            if (tryCatch.HasCaught()) {
                node::FatalException(tryCatch);
            }
        }

        if (request->buffer && !request->cbEach.IsEmpty()) {
            uint64_t index = 0;
            for (std::vector<std::string**>::iterator iterator = request->rows->begin(), end = request->rows->end(); iterator != end; ++iterator, index++) {
                v8::Local<v8::Value> eachArgv[3];

                eachArgv[0] = rows->Get(index);
                eachArgv[1] = v8::Number::New(index);
                eachArgv[2] = v8::Local<v8::Value>::New(iterator == end ? v8::True() : v8::False());

                v8::TryCatch tryCatch;
                request->cbEach->Call(v8::Context::GetCurrent()->Global(), 3, eachArgv);
                if (tryCatch.HasCaught()) {
                    node::FatalException(tryCatch);
                }
            }
        }
    } else if (!request->cbError.IsEmpty()) {
        v8::Local<v8::Value> argv[1];
        argv[0] = v8::String::New(request->error != NULL ? request->error : "(unknown error)");

        v8::TryCatch tryCatch;
        request->cbError->Call(v8::Context::GetCurrent()->Global(), 1, argv);
        if (tryCatch.HasCaught()) {
            node::FatalException(tryCatch);
        }
    }

    eioQueryCleanup(request);

    return 0;
}

int node_drizzle::Drizzle::eioQueryEach(eio_req* eioRequest) {
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

            for (uint16_t i = 0; i < columnCount; i++) {
                if (currentRow[i] != NULL) {
                    row[i] = new std::string(currentRow[i]);
                } else {
                    row[i] = NULL;
                }
            }

            request->rows->push_back(row);
        }
    } catch(const drizzle::Exception& exception) {
        if (request->rows != NULL) {
            delete request->rows;
        }
        request->error = exception.what();
    }

    return 0;
}

int node_drizzle::Drizzle::eioQueryEachFinished(eio_req* eioRequest) {
    v8::HandleScope scope;

    query_request_t *request = static_cast<query_request_t *>(eioRequest->data);
    assert(request);

    if (!request->cbEach.IsEmpty() && request->rows != NULL) {
        v8::Local<v8::Value> eachArgv[3];

        eachArgv[0] = request->drizzle->row(request->result, request->rows->front(), request->cast);
        eachArgv[1] = v8::Number::New(request->result->index());
        eachArgv[2] = v8::Local<v8::Value>::New(request->result->hasNext() ? v8::False() : v8::True());

        v8::TryCatch tryCatch;
        request->cbEach->Call(v8::Context::GetCurrent()->Global(), 3, eachArgv);
        if (tryCatch.HasCaught()) {
            node::FatalException(tryCatch);
        }
    }

    eioQueryCleanup(request);

    return 0;
}

void node_drizzle::Drizzle::eioQueryCleanup(query_request_t* request) {
    ev_unref(EV_DEFAULT_UC);
    request->drizzle->Unref();

    if (request->result == NULL || !request->result->hasNext()) {
        if (!request->cbFinish.IsEmpty()) {
            v8::TryCatch tryCatch;
            request->cbFinish->Call(v8::Context::GetCurrent()->Global(), 0, NULL);
            if (tryCatch.HasCaught()) {
                node::FatalException(tryCatch);
            }
        }

        eioQueryRequestFree(request);
    } else {
        request->drizzle->Ref();
        eio_custom(eioQueryEach, EIO_PRI_DEFAULT, eioQueryEachFinished, request);
        ev_ref(EV_DEFAULT_UC);
    }
}

void node_drizzle::Drizzle::eioQueryRequestFree(query_request_t* request) {
    if (request->result != NULL) {
        if (request->rows != NULL) {
            uint16_t columnCount = request->result->columnCount();
            for (std::vector<std::string**>::iterator iterator = request->rows->begin(), end = request->rows->end(); iterator != end; ++iterator) {
                std::string** row = *iterator;
                for (uint16_t i = 0; i < columnCount; i++) {
                    if (row[i] != NULL) {
                        delete row[i];
                    }
                }
                delete [] row;
            }

            delete request->rows;
        }

        delete request->result;
    }

    request->cbStart.Dispose();
    request->cbFinish.Dispose();
    request->cbSuccess.Dispose();
    request->cbError.Dispose();
    request->cbEach.Dispose();

    delete request;
}

v8::Local<v8::Object> node_drizzle::Drizzle::row(drizzle::Result* result, std::string** currentRow, bool cast) const {
    v8::Local<v8::Object> row = v8::Object::New();

    for (uint16_t j = 0, limitj = result->columnCount(); j < limitj; j++) {
        drizzle::Result::Column* currentColumn = result->column(j);
        v8::Local<v8::Value> value;

        if (currentRow[j] != NULL) {
            const char* currentValue = currentRow[j]->c_str();
            if (cast) {
                switch (currentColumn->getType()) {
                    case drizzle::Result::Column::BOOL:
                        value = v8::Local<v8::Value>::New(currentRow[j]->empty() || currentRow[j]->compare("0") != 0 ? v8::True() : v8::False());
                        break;
                    case drizzle::Result::Column::INT:
                        value = v8::String::New(currentValue)->ToInteger();
                        break;
                    case drizzle::Result::Column::NUMBER:
                        value = v8::Number::New(::atof(currentValue));
                        break;
                    case drizzle::Result::Column::DATE:
                        try {
                            value = v8::Date::New(this->toDate(*currentRow[j], false));
                        } catch(const drizzle::Exception&) {
                            value = v8::String::New(currentValue);
                        }
                        break;
                    case drizzle::Result::Column::TIME:
                        value = v8::Date::New(this->toTime(*currentRow[j]));
                        break;
                    case drizzle::Result::Column::DATETIME:
                        try {
                            value = v8::Date::New(this->toDate(*currentRow[j], true));
                        } catch(const drizzle::Exception&) {
                            value = v8::String::New(currentValue);
                        }
                        break;
                    case drizzle::Result::Column::TEXT:
                        value = v8::Local<v8::Value>::New(node::Buffer::New(v8::String::New(currentValue, currentRow[j]->length())));
                        break;
                    case drizzle::Result::Column::SET:
                        {
                            v8::Local<v8::Array> values = v8::Array::New();
                            std::istringstream stream(*currentRow[j]);
                            std::string item;
                            uint64_t index = 0;
                            while (std::getline(stream, item, ',')) {
                                if (!item.empty()) {
                                    values->Set(v8::Integer::New(index++), v8::String::New(item.c_str()));
                                }
                            }
                            value = values;
                        }
                        break;
                    default:
                        value = v8::String::New(currentValue);
                        break;
                }
            } else {
                value = v8::String::New(currentValue);
            }
        } else {
            value = v8::Local<v8::Value>::New(v8::Null());
        }
        row->Set(v8::String::New(currentColumn->getName().c_str()), value);
    }

    return row;
}

std::string node_drizzle::Drizzle::parseQuery(const std::string& query, v8::Local<v8::Array> values) const throw(drizzle::Exception&) {
    std::string parsed(query);
    std::vector<std::string::size_type> positions;
    char quote = 0;
    bool escaped = false;
    uint32_t delta = 0;

    for (std::string::size_type i = 0, limiti = query.length(); i < limiti; i++) {
        char currentChar = query[i];
        if (escaped) {
            if (currentChar == '?') {
                parsed.replace(i - 1 - delta, 1, "");
                delta++;
            }
            escaped = false;
        } else if (currentChar == '\\') {
            escaped = true;
        } else if (quote && currentChar == quote) {
            quote = 0;
        } else if (!quote && (currentChar == '\'' || currentChar == '"')) {
            quote = currentChar;
        } else if (!quote && currentChar == '?') {
            positions.push_back(i - delta);
        }
    }

    if (positions.size() != values->Length()) {
        throw drizzle::Exception("Wrong number of values to escape");
    }

    uint32_t index = 0;
    delta = 0;
    for (std::vector<std::string::size_type>::iterator iterator = positions.begin(), end = positions.end(); iterator != end; ++iterator, index++) {
        std::string value = this->value(values->Get(index));
        parsed.replace(*iterator + delta, 1, value);
        delta += (value.length() - 1);
    }

    return parsed;
}

std::string node_drizzle::Drizzle::value(v8::Local<v8::Value> value, bool inArray) const throw(drizzle::Exception&) {
    std::ostringstream currentStream;

    if (value->IsArray()) {
        v8::Local<v8::Array> array = v8::Array::Cast(*value);
        if (!inArray) {
            currentStream << "(";
        }
        for (uint32_t i = 0, limiti = array->Length(); i < limiti; i++) {
            v8::Local<v8::Value> child = array->Get(i);
            if (child->IsArray() && i > 0) {
                currentStream << "),(";
            } else if (i > 0) {
                currentStream << ",";
            }

            currentStream << this->value(child, true);
        }
        if (!inArray) {
            currentStream << ")";
        }
    } else if (value->IsDate()) {
        currentStream << '\'' <<  this->fromDate(v8::Date::Cast(*value)->NumberValue()) << '\'';
    } else if (value->IsBoolean()) {
        currentStream << (value->IsTrue() ? "1" : "0");
    } else if (value->IsNumber()) {
        currentStream << value->ToNumber()->Value();
    } else if (value->IsString()) {
        v8::String::Utf8Value currentString(value->ToString());
        std::string string = *currentString;
        currentStream << '\'' <<  this->connection.escape(string) << '\'';
    }

    return currentStream.str();
}

uint64_t node_drizzle::Drizzle::toDate(const std::string& value, bool hasTime) const throw(drizzle::Exception&) {
    int day, month, year, hour, min, sec;
    char sep;
    std::istringstream stream(value, std::istringstream::in);

    if (hasTime) {
        stream >> year >> sep >> month >> sep >> day >> hour >> sep >> min >> sep >> sec;
    } else {
        stream >> year >> sep >> month >> sep >> day;
        hour = min = sec = 0;
    }

    time_t rawtime;
    struct tm timeinfo;

    time(&rawtime);
    if (!localtime_r(&rawtime, &timeinfo)) {
        throw drizzle::Exception("Can't get local time");
    }

    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = min;
    timeinfo.tm_sec = sec;

    return static_cast<uint64_t>((mktime(&timeinfo) + this->gmtDelta()) * 1000);
}

std::string node_drizzle::Drizzle::fromDate(const uint64_t timeStamp) const throw(drizzle::Exception&) {
    char* buffer = new char[20];
    if (buffer == NULL) {
        throw drizzle::Exception("Can\'t create buffer to write parsed date");
    }


    struct tm timeinfo;
    time_t rawtime = timeStamp / 1000;
    if (!localtime_r(&rawtime, &timeinfo)) {
        throw drizzle::Exception("Can't get local time");
    }

    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", &timeinfo);

    std::string date(buffer);
    delete [] buffer;

    return date;
}

int node_drizzle::Drizzle::gmtDelta() const throw(drizzle::Exception&) {
    int localHour, gmtHour, localMin, gmtMin;
    time_t rawtime;
    struct tm timeinfo;

    time(&rawtime);
    if (!localtime_r(&rawtime, &timeinfo)) {
        throw drizzle::Exception("Can't get local time");
    }
    localHour = timeinfo.tm_hour - (timeinfo.tm_isdst > 0 ? 1 : 0);
    localMin = timeinfo.tm_min;

    if (!gmtime_r(&rawtime, &timeinfo)) {
        throw drizzle::Exception("Can't get GMT time");
    }
    gmtHour = timeinfo.tm_hour;
    gmtMin = timeinfo.tm_min;

    int gmtDelta = ((localHour - gmtHour) * 60 + (localMin - gmtMin)) * 60;
    if (gmtDelta <= -(12 * 60 * 60)) {
        gmtDelta += 24 * 60 * 60;
    } else if (gmtDelta > (12 * 60 * 60)) {
        gmtDelta -= 24 * 60 * 60;
    }

    return gmtDelta;
}

uint64_t node_drizzle::Drizzle::toTime(const std::string& value) const {
    int hour, min, sec;
    char sep;
    std::istringstream stream(value, std::istringstream::in);

    stream >> hour >> sep >> min >> sep >> sec;

    time_t rawtime;
    struct tm timeinfo;

    time(&rawtime);
    if (!localtime_r(&rawtime, &timeinfo)) {
        throw drizzle::Exception("Can't get local time");
    }

    timeinfo.tm_hour = hour;
    timeinfo.tm_min = min;
    timeinfo.tm_sec = sec;

    return static_cast<uint64_t>((mktime(&timeinfo) + this->gmtDelta()) * 1000);
}

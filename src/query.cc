// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./query.h"

v8::Persistent<v8::FunctionTemplate> node_drizzle::Query::constructorTemplate;
v8::Persistent<v8::String> node_drizzle::Query::sySuccess;
v8::Persistent<v8::String> node_drizzle::Query::syError;
v8::Persistent<v8::String> node_drizzle::Query::syEach;

void node_drizzle::Query::Init(v8::Handle<v8::Object> target) {
    v8::HandleScope scope;

    v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);

    constructorTemplate = v8::Persistent<v8::FunctionTemplate>::New(t);
    constructorTemplate->Inherit(node::EventEmitter::constructor_template);
    constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_ADD_PROTOTYPE_METHOD(constructorTemplate, "execute", Execute);

    target->Set(v8::String::NewSymbol("Query"), constructorTemplate->GetFunction());

    sySuccess = NODE_PERSISTENT_SYMBOL("success");
    syError = NODE_PERSISTENT_SYMBOL("error");
    syEach = NODE_PERSISTENT_SYMBOL("each");
}

node_drizzle::Query::Query(): node::EventEmitter(),
    connection(NULL), cast(true), cbStart(NULL), cbFinish(NULL) {
}

node_drizzle::Query::~Query() {
    this->values.Dispose();
    if (this->cbStart != NULL) {
        node::cb_destroy(this->cbStart);
    }
    if (this->cbFinish != NULL) {
        node::cb_destroy(this->cbFinish);
    }
}

v8::Handle<v8::Value> node_drizzle::Query::New(const v8::Arguments& args) {
    v8::HandleScope scope;

    node_drizzle::Query *query = new node_drizzle::Query();
    if (query == NULL) {
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Can't create query object")));
    }

    if (args.Length() > 0) {
        v8::Handle<v8::Value> set = query->set(args);
        if (!set.IsEmpty()) {
            return set;
        }
    }

    query->Wrap(args.This());

    return args.This();
}

void node_drizzle::Query::setConnection(drizzle::Connection* connection) {
    this->connection = connection;
}

v8::Handle<v8::Value> node_drizzle::Query::Execute(const v8::Arguments& args) {
    v8::HandleScope scope;

    node_drizzle::Query *query = node::ObjectWrap::Unwrap<node_drizzle::Query>(args.This());
    assert(query);

    if (args.Length() > 0) {
        v8::Handle<v8::Value> set = query->set(args);
        if (!set.IsEmpty()) {
            return set;
        }
    }

    try {
        query->sql = query->parseQuery(query->sql, query->values);
    } catch(const drizzle::Exception& exception) {
        return v8::ThrowException(v8::Exception::Error(v8::String::New(exception.what())));
    }

    if (query->cbStart != NULL && !query->cbStart->IsEmpty()) {
        v8::Local<v8::Value> argv[1];
        argv[0] = v8::String::New(query->sql.c_str());

        v8::TryCatch tryCatch;
        v8::Handle<v8::Value> result = (*(query->cbStart))->Call(v8::Context::GetCurrent()->Global(), 1, argv);
        if (tryCatch.HasCaught()) {
            node::FatalException(tryCatch);
        }

        if (!result->IsUndefined()) {
            if (result->IsFalse()) {
                return v8::Undefined();
            } else if (result->IsString()) {
                v8::String::Utf8Value modifiedQuery(result->ToString());
                query->sql = *modifiedQuery;
            }
        }
    }

    execute_request_t *request = new execute_request_t();
    if (request == NULL) {
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Could not create EIO request")));
    }

    request->query = query;
    request->result = NULL;
    request->rows = NULL;
    request->error = NULL;

    request->query->Ref();
    eio_custom(eioExecute, EIO_PRI_DEFAULT, eioExecuteFinished, request);
    ev_ref(EV_DEFAULT_UC);

    return v8::Undefined();
}

int node_drizzle::Query::eioExecute(eio_req* eioRequest) {
    execute_request_t *request = static_cast<execute_request_t *>(eioRequest->data);
    assert(request);
    assert(request->query->connection);

    try {
        request->result = request->query->connection->query(request->query->sql);
        if (request->result != NULL) {
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

int node_drizzle::Query::eioExecuteFinished(eio_req* eioRequest) {
    v8::HandleScope scope;

    execute_request_t *request = static_cast<execute_request_t *>(eioRequest->data);
    assert(request);

    uint16_t columnCount = (request->result != NULL ? request->result->columnCount() : 0);
    if (request->error == NULL && request->result != NULL) {
        assert(request->rows);

        v8::Local<v8::Value> argv[2];
        uint8_t argc = 1;

        v8::Local<v8::Array> columns = v8::Array::New(columnCount);
        for (uint16_t j = 0; j < columnCount; j++) {
            drizzle::Result::Column *currentColumn = request->result->column(j);
            v8::Local<v8::Value> columnType;

            v8::Local<v8::Object> column = v8::Object::New();
            column->Set(v8::String::New("name"), v8::String::New(currentColumn->getName().c_str()));
            column->Set(v8::String::New("type"), NODE_CONSTANT(currentColumn->getType()));

            columns->Set(j, column);
        }

        argv[0] = columns;

        v8::Local<v8::Array> rows = v8::Array::New();

        uint64_t index = 0;
        for (std::vector<std::string**>::iterator iterator = request->rows->begin(), end = request->rows->end(); iterator != end; ++iterator, index++) {
            std::string** currentRow = *iterator;
            v8::Local<v8::Object> row = request->query->row(request->result, currentRow, request->query->cast);

            v8::Local<v8::Value> eachArgv[3];

            eachArgv[0] = row;
            eachArgv[1] = v8::Number::New(index);
            eachArgv[2] = v8::Local<v8::Value>::New(iterator == end ? v8::True() : v8::False());

            request->query->Emit(syEach, 3, eachArgv);

            rows->Set(index, row);
        }

        argc = 2;
        argv[1] = rows;

        request->query->Emit(sySuccess, argc, argv);

        for (std::vector<std::string**>::iterator iterator = request->rows->begin(), end = request->rows->end(); iterator != end; ++iterator, index++) {
        }
    } else {
        v8::Local<v8::Value> argv[1];
        argv[0] = v8::String::New(request->error != NULL ? request->error : "(unknown error)");

        request->query->Emit(syError, 1, argv);
    }

    ev_unref(EV_DEFAULT_UC);
    request->query->Unref();

    if (request->query->cbFinish != NULL && !request->query->cbFinish->IsEmpty()) {
        v8::TryCatch tryCatch;
        (*(request->query->cbFinish))->Call(v8::Context::GetCurrent()->Global(), 0, NULL);
        if (tryCatch.HasCaught()) {
            node::FatalException(tryCatch);
        }
    }

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

    delete request;

    return 0;
}

v8::Handle<v8::Value> node_drizzle::Query::set(const v8::Arguments& args) {
    if (args.Length() == 0) {
        return v8::Handle<v8::Value>();
    }

    int queryIndex = -1, optionsIndex = -1, valuesIndex = -1;

    if (args.Length() > 2) {
        ARG_CHECK_STRING(0, query);
        ARG_CHECK_ARRAY(1, values);
        ARG_CHECK_OBJECT(2, options);
        queryIndex = 0;
        valuesIndex = 1;
        optionsIndex = 2;
    } else if (args.Length() > 1) {
        ARG_CHECK_STRING(0, query);
        queryIndex = 0;
        if (args[1]->IsArray()) {
            ARG_CHECK_ARRAY(1, values);
            valuesIndex = 1;
        } else {
            ARG_CHECK_OBJECT(1, options);
            optionsIndex = 1;
        }
    } else if (args[0]->IsString()) {
        ARG_CHECK_STRING(0, query);
        queryIndex = 0;
    } else if (args[0]->IsArray()) {
        ARG_CHECK_ARRAY(0, values);
        valuesIndex = 0;
    } else {
        ARG_CHECK_OBJECT(0, options);
        optionsIndex = 0;
    }

    if (queryIndex >= 0) {
        v8::String::Utf8Value sql(args[queryIndex]->ToString());
        this->sql = *sql;
    }

    if (optionsIndex >= 0) {
        v8::Local<v8::Object> options = args[optionsIndex]->ToObject();

        ARG_CHECK_OBJECT_ATTR_OPTIONAL_BOOL(options, cast);
        ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, start);
        ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, finish);
        ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, success);
        ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, error);
        ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(options, each);

        if (options->Has(cast_key)) {
            this->cast = options->Get(cast_key)->IsTrue();
        }

        if (options->Has(start_key)) {
            if (this->cbStart != NULL) {
                node::cb_destroy(this->cbStart);
            }
            this->cbStart = node::cb_persist(options->Get(start_key));
        }

        if (options->Has(finish_key)) {
            if (this->cbFinish != NULL) {
                node::cb_destroy(this->cbFinish);
            }
            this->cbFinish = node::cb_persist(options->Get(finish_key));
        }
    }

    if (valuesIndex >= 0) {
        this->values = v8::Array::Cast(*args[valuesIndex]);
    }

    return v8::Handle<v8::Value>();
}

v8::Local<v8::Object> node_drizzle::Query::row(drizzle::Result* result, std::string** currentRow, bool cast) const {
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

std::string node_drizzle::Query::parseQuery(const std::string& query, v8::Persistent<v8::Array> values) const throw(drizzle::Exception&) {
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

    uint32_t valuesLength = !values.IsEmpty() ? values->Length() : 0;
    if (positions.size() != valuesLength) {
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

std::string node_drizzle::Query::value(v8::Local<v8::Value> value, bool inArray) const throw(drizzle::Exception&) {
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
        currentStream << '\'' <<  this->connection->escape(string) << '\'';
    }

    return currentStream.str();
}

uint64_t node_drizzle::Query::toDate(const std::string& value, bool hasTime) const throw(drizzle::Exception&) {
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

std::string node_drizzle::Query::fromDate(const uint64_t timeStamp) const throw(drizzle::Exception&) {
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

int node_drizzle::Query::gmtDelta() const throw(drizzle::Exception&) {
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

uint64_t node_drizzle::Query::toTime(const std::string& value) const {
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


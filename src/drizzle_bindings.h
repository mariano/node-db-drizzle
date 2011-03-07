#ifndef _NODE_DRIZZLE__DRIZZLE_BINDINGS_H
#define _NODE_DRIZZLE__DRIZZLE_BINDINGS_H

#include <node.h>

using namespace v8;

#define NODE_CONSTANT(constant) Integer::New(constant)

#define ARG_CHECK_OPTIONAL_STRING(I, VAR) \
    if (args.Length() > I && !args[I]->IsString()) { \
        return ThrowException(Exception::Error(String::New("Argument \"" #VAR "\" must be a valid string"))); \
    }

#define ARG_CHECK_STRING(I, VAR) \
    if (args.Length() <= I) { \
        return ThrowException(Exception::Error(String::New("Argument \"" #VAR "\" is mandatory"))); \
    } else if (!args[I]->IsString()) { \
        return ThrowException(Exception::Error(String::New("Argument \"" #VAR "\" must be a valid string"))); \
    }

#define ARG_CHECK_OPTIONAL_OBJECT(I, VAR) \
    if (args.Length() > I && (!args[I]->IsObject() || args[I]->IsFunction() || args[I]->IsUndefined())) { \
        return ThrowException(Exception::Error(String::New("Argument \"" #VAR "\" must be a valid object"))); \
    }

#define ARG_CHECK_OBJECT(I, VAR) \
    if (args.Length() <= I) { \
        return ThrowException(Exception::Error(String::New("Argument \"" #VAR "\" is mandatory"))); \
    } else if (!args[I]->IsObject() || args[I]->IsFunction() || args[I]->IsUndefined()) { \
        return ThrowException(Exception::Error(String::New("Argument \"" #VAR "\" must be a valid object"))); \
    }

#define ARG_CHECK_OPTIONAL_FUNCTION(I, VAR) \
    if (args.Length() > I && !args[I]->IsFunction()) { \
        return ThrowException(Exception::Error(String::New("Argument \"" #VAR "\" must be a valid function"))); \
    }

#define ARG_CHECK_FUNCTION(I, VAR) \
    if (args.Length() <= I) { \
        return ThrowException(Exception::Error(String::New("Argument \"" #VAR "\" is mandatory"))); \
    } else if (!args[I]->IsFunction()) { \
        return ThrowException(Exception::Error(String::New("Argument \"" #VAR "\" must be a valid function"))); \
    }

#define ARG_CHECK_OBJECT_ATTR_STRING(VAR, KEY) \
    Local<String> KEY##_##key = String::New("" #KEY ""); \
    if (!VAR->Has(KEY##_##key)) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" is mandatory"))); \
    } else if (!VAR->Get(KEY##_##key)->IsString()) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" must be a valid string"))); \
    }

#define ARG_CHECK_OBJECT_ATTR_OPTIONAL_STRING(VAR, KEY) \
    Local<String> KEY##_##key = String::New("" #KEY ""); \
    if (VAR->Has(KEY##_##key) && !VAR->Get(KEY##_##key)->IsString()) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" must be a valid string"))); \
    }

#define ARG_CHECK_OBJECT_ATTR_UINT32(VAR, KEY) \
    Local<String> KEY##_##key = String::New("" #KEY ""); \
    if (!VAR->Has(KEY##_##key)) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" is mandatory"))); \
    } else if (!VAR->Get(KEY##_##key)->IsUint32()) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" must be a valid UINT32"))); \
    }

#define ARG_CHECK_OBJECT_ATTR_OPTIONAL_UINT32(VAR, KEY) \
    Local<String> KEY##_##key = String::New("" #KEY ""); \
    if (VAR->Has(KEY##_##key) && !VAR->Get(KEY##_##key)->IsUint32()) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" must be a valid UINT32"))); \
    }

#define ARG_CHECK_OBJECT_ATTR_BOOL(VAR, KEY) \
    Local<String> KEY##_##key = String::New("" #KEY ""); \
    if (!VAR->Has(KEY##_##key)) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" is mandatory"))); \
    } else if (!VAR->Get(KEY##_##key)->IsBoolean()) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" must be a valid boolean"))); \
    }

#define ARG_CHECK_OBJECT_ATTR_OPTIONAL_BOOL(VAR, KEY) \
    Local<String> KEY##_##key = String::New("" #KEY ""); \
    if (VAR->Has(KEY##_##key) && !VAR->Get(KEY##_##key)->IsBoolean()) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" must be a valid boolean"))); \
    }

#define ARG_CHECK_OBJECT_ATTR_FUNCTION(VAR, KEY) \
    Local<String> KEY##_##key = String::New("" #KEY ""); \
    if (!VAR->Has(KEY##_##key)) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" is mandatory"))); \
    } else if (!VAR->Get(KEY##_##key)->IsFunction()) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" must be a valid function"))); \
    }

#define ARG_CHECK_OBJECT_ATTR_OPTIONAL_FUNCTION(VAR, KEY) \
    Local<String> KEY##_##key = String::New("" #KEY ""); \
    if (VAR->Has(KEY##_##key) && !VAR->Get(KEY##_##key)->IsFunction()) { \
        return ThrowException(Exception::Error(String::New("Option \"" #KEY "\" must be a valid function"))); \
    }


#endif

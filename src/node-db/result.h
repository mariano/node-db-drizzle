// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#ifndef SRC_NODE_DB_RESULT_H_
#define SRC_NODE_DB_RESULT_H_

#include <stdint.h>
#include <stdexcept>
#include "./exception.h"

namespace node_db {
class Result {
    public:
        class Column {
            public:
                typedef enum {
                    STRING,
                    TEXT,
                    INT,
                    NUMBER,
                    DATE,
                    TIME,
                    DATETIME,
                    BOOL,
                    SET
                } type_t;

                virtual std::string getName() const = 0;
                virtual type_t getType() const = 0;
        };

        virtual bool hasNext() const = 0;
        virtual const char** next() throw(Exception&) = 0;
        virtual uint64_t index() const throw(std::out_of_range&) = 0;
        virtual Column* column(uint16_t i) const throw(std::out_of_range&) = 0;
        virtual uint64_t insertId() const = 0;
        virtual uint16_t columnCount() const = 0;
};
}

#endif  // SRC_NODE_DB_RESULT_H_

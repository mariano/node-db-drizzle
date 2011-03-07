#ifndef _DRIZZLE__RESULT_H
#define _DRIZZLE__RESULT_H

#include <string>
#include <stdexcept>
#include <libdrizzle/drizzle.h>
#include <libdrizzle/drizzle_client.h>
#include "exception.h"

namespace drizzle {
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

                Column(drizzle_column_st *column);
                ~Column();
                std::string getName() const;
                type_t getType() const;

            protected:
                std::string name;
                type_t type;
        };

        Result(drizzle_st* drizzle, drizzle_result_st* result) throw(Exception&);
        ~Result();
        bool hasNext() const;
        const char** next() throw(Exception&);
        uint64_t index() const throw(std::out_of_range&);
        Column* column(uint16_t i) const throw(std::out_of_range&);
        uint64_t insertId() const;
        uint64_t affectedCount() const;
        uint16_t warningCount() const;
        uint16_t columnCount() const;

    protected:
        Column **columns;
        uint16_t totalColumns;
        uint64_t rowNumber;

        char **row() throw(Exception&);

    private:
        drizzle_st *drizzle;
        drizzle_result_st *result;
        drizzle_row_t previousRow;
        drizzle_row_t nextRow;
};
}

#endif

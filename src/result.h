// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#ifndef SRC_RESULT_H_
#define SRC_RESULT_H_

#include <libdrizzle/drizzle.h>
#include <libdrizzle/drizzle_client.h>
#include <string>
#include <stdexcept>
#include "./node-db/exception.h"
#include "./node-db/result.h"

namespace node_db_drizzle {
class Result : public node_db::Result {
    public:
        class Column : public node_db::Result::Column {
            public:
                explicit Column(drizzle_column_st *column);
                ~Column();
                std::string getName() const;
                node_db::Result::Column::type_t getType() const;

            protected:
                std::string name;
                type_t type;
        };

        explicit Result(drizzle_st* drizzle, drizzle_result_st* result) throw(node_db::Exception&);
        ~Result();
        bool hasNext() const;
        const char** next() throw(node_db::Exception&);
        uint64_t index() const throw(std::out_of_range&);
        Column* column(uint16_t i) const throw(std::out_of_range&);
        uint64_t insertId() const;
        uint16_t columnCount() const;
        uint64_t affectedCount() const;
        uint16_t warningCount() const;

    protected:
        Column **columns;
        uint16_t totalColumns;
        uint64_t rowNumber;

        char **row() throw(node_db::Exception&);

    private:
        drizzle_st *drizzle;
        drizzle_result_st *result;
        drizzle_row_t previousRow;
        drizzle_row_t nextRow;
};
}

#endif  // SRC_RESULT_H_

#include "result.h"

using namespace drizzle;

Result::Column::Column(drizzle_column_st *column) {
    this->name = drizzle_column_name(column);

    switch(drizzle_column_type(column)) {
        case DRIZZLE_COLUMN_TYPE_TINY:
            this->type = (drizzle_column_size(column) == 1 ? BOOL : INT);
            break;
        case DRIZZLE_COLUMN_TYPE_BIT:
        case DRIZZLE_COLUMN_TYPE_SHORT:
        case DRIZZLE_COLUMN_TYPE_YEAR:
        case DRIZZLE_COLUMN_TYPE_INT24:
        case DRIZZLE_COLUMN_TYPE_LONG:
        case DRIZZLE_COLUMN_TYPE_LONGLONG:
            this->type = INT;
            break;
        case DRIZZLE_COLUMN_TYPE_FLOAT:
        case DRIZZLE_COLUMN_TYPE_DOUBLE:
        case DRIZZLE_COLUMN_TYPE_DECIMAL:
        case DRIZZLE_COLUMN_TYPE_NEWDECIMAL:
            this->type = NUMBER;
            break;
        case DRIZZLE_COLUMN_TYPE_DATE:
        case DRIZZLE_COLUMN_TYPE_NEWDATE:
            this->type = DATE;
            break;
        case DRIZZLE_COLUMN_TYPE_TIME:
            this->type = TIME;
            break;
        case DRIZZLE_COLUMN_TYPE_TIMESTAMP:
        case DRIZZLE_COLUMN_TYPE_DATETIME:
            this->type = DATETIME;
            break;
        case DRIZZLE_COLUMN_TYPE_TINY_BLOB:
        case DRIZZLE_COLUMN_TYPE_MEDIUM_BLOB:
        case DRIZZLE_COLUMN_TYPE_LONG_BLOB:
        case DRIZZLE_COLUMN_TYPE_BLOB:
            this->type = TEXT;
            break;
        case DRIZZLE_COLUMN_TYPE_SET:
            this->type = SET;
            break;
        default:
            this->type = STRING;
            break;
    }
}

Result::Column::~Column() {
}

std::string Result::Column::getName() const {
    return this->name;
}

Result::Column::type_t Result::Column::getType() const {
    return this->type;
}

Result::Result(drizzle_st* drizzle, drizzle_result_st* result) throw(Exception&):
    columns(NULL),
    totalColumns(0),
    rowNumber(0),
    drizzle(drizzle),
    result(result),
    previousRow(NULL),
    nextRow(NULL)
{
    if (this->result == NULL) {
        throw Exception("Invalid result");
    }

    if (drizzle_column_buffer(this->result) != DRIZZLE_RETURN_OK) {
        drizzle_result_free(this->result);
        throw Exception("Could not buffer columns");
    }

    this->totalColumns = drizzle_result_column_count(this->result);
    if (this->totalColumns > 0) {
        this->columns = new Column*[this->totalColumns];
        if (this->columns == NULL) {
            drizzle_result_free(this->result);
            throw Exception("Could not allocate storage for columns");
        }

        uint16_t i=0;
        drizzle_column_st *current;
        while ((current = drizzle_column_next(this->result)) != NULL) {
            this->columns[i++] = new Column(current);
        }
    }

    this->nextRow = this->row();
}

Result::~Result() {
    if (this->columns != NULL) {
        for (uint16_t i=0; i < this->totalColumns; i++) {
            delete this->columns[i];
        }
        delete [] this->columns;
    }
    if (this->result != NULL) {
        if (this->previousRow != NULL) {
            drizzle_row_free(this->result, this->previousRow);
        }
        if (this->nextRow != NULL) {
            drizzle_row_free(this->result, this->nextRow);
        }
        drizzle_result_free(this->result);
    }
}

bool Result::hasNext() const {
    return (this->nextRow != NULL);
}

const char** Result::next() throw(Exception&) {
    if (this->previousRow != NULL) {
        drizzle_row_free(this->result, this->previousRow);
    }

    if (this->nextRow == NULL) {
        return NULL;
    }

    this->rowNumber++;
    this->previousRow = this->nextRow;
    this->nextRow = this->row();

    return (const char**) this->previousRow;
}

char** Result::row() throw(Exception&) {
    drizzle_return_t result = DRIZZLE_RETURN_IO_WAIT;
    char **row = NULL;

    while (result == DRIZZLE_RETURN_IO_WAIT) {
        row = drizzle_row_buffer(this->result, &result);
        if (result == DRIZZLE_RETURN_IO_WAIT) {
            if (drizzle_con_wait(this->drizzle) != DRIZZLE_RETURN_OK) {
                throw Exception("Could not wait for connection");
            }

            if (drizzle_con_ready(this->drizzle) == NULL) {
                throw Exception("Could not fetch connection");
            }
        }
    }

    if (result != DRIZZLE_RETURN_OK) {
        if (row != NULL) {
            drizzle_row_free(this->result, row);
            row = NULL;
        }
        throw Exception("Could not prefetch next row");
    }

    return row;
}

uint64_t Result::index() const throw(std::out_of_range&) {
    if (this->rowNumber == 0) {
        throw std::out_of_range("Not standing on a row");
    }
    return (this->rowNumber - 1);
}

Result::Column* Result::column(uint16_t i) const throw(std::out_of_range&) {
    if (i < 0 || i >= this->totalColumns) {
        throw std::out_of_range("Wrong column index");
    }
    return this->columns[i];
}

uint64_t Result::insertId() const {
    return drizzle_result_insert_id(this->result);
}

uint64_t Result::affectedCount() const {
    return drizzle_result_affected_rows(this->result);
}

uint16_t Result::warningCount() const {
    return drizzle_result_warning_count(this->result);
}

uint16_t Result::columnCount() const {
    return this->totalColumns;
}

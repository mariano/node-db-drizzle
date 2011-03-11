// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./result.h"

drizzle::Result::Column::Column(drizzle_column_st *column) {
    this->name = drizzle_column_name(column);

    switch (drizzle_column_type(column)) {
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

drizzle::Result::Column::~Column() {
}

std::string drizzle::Result::Column::getName() const {
    return this->name;
}

drizzle::Result::Column::type_t drizzle::Result::Column::getType() const {
    return this->type;
}

drizzle::Result::Result(drizzle_st* drizzle, drizzle_result_st* result) throw(drizzle::Exception&)
    : columns(NULL),
    totalColumns(0),
    rowNumber(0),
    drizzle(drizzle),
    result(result),
    previousRow(NULL),
    nextRow(NULL) {
    if (this->result == NULL) {
        throw drizzle::Exception("Invalid result");
    }

    if (drizzle_column_buffer(this->result) != DRIZZLE_RETURN_OK) {
        throw drizzle::Exception("Could not buffer columns");
    }

    this->totalColumns = drizzle_result_column_count(this->result);
    if (this->totalColumns > 0) {
        this->columns = new Column*[this->totalColumns];
        if (this->columns == NULL) {
            throw drizzle::Exception("Could not allocate storage for columns");
        }

        uint16_t i = 0;
        drizzle_column_st *current;
        while ((current = drizzle_column_next(this->result)) != NULL) {
            this->columns[i++] = new Column(current);
        }
    }

    this->nextRow = this->row();
}

drizzle::Result::~Result() {
    if (this->columns != NULL) {
        for (uint16_t i = 0; i < this->totalColumns; i++) {
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
    }
}

bool drizzle::Result::hasNext() const {
    return (this->nextRow != NULL);
}

const char** drizzle::Result::next() throw(drizzle::Exception&) {
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

char** drizzle::Result::row() throw(drizzle::Exception&) {
    drizzle_return_t result = DRIZZLE_RETURN_IO_WAIT;
    char **row = NULL;

    while (result == DRIZZLE_RETURN_IO_WAIT) {
        row = drizzle_row_buffer(this->result, &result);
        if (result == DRIZZLE_RETURN_IO_WAIT) {
            if (drizzle_con_wait(this->drizzle) != DRIZZLE_RETURN_OK) {
                throw drizzle::Exception("Could not wait for connection");
            }

            if (drizzle_con_ready(this->drizzle) == NULL) {
                throw drizzle::Exception("Could not fetch connection");
            }
        }
    }

    if (result != DRIZZLE_RETURN_OK) {
        if (row != NULL) {
            drizzle_row_free(this->result, row);
            row = NULL;
        }
        throw drizzle::Exception("Could not prefetch next row");
    }

    return row;
}

uint64_t drizzle::Result::index() const throw(std::out_of_range&) {
    if (this->rowNumber == 0) {
        throw std::out_of_range("Not standing on a row");
    }
    return (this->rowNumber - 1);
}

drizzle::Result::Column* drizzle::Result::column(uint16_t i) const throw(std::out_of_range&) {
    if (i < 0 || i >= this->totalColumns) {
        throw std::out_of_range("Wrong column index");
    }
    return this->columns[i];
}

uint64_t drizzle::Result::insertId() const {
    return drizzle_result_insert_id(this->result);
}

uint64_t drizzle::Result::affectedCount() const {
    return drizzle_result_affected_rows(this->result);
}

uint16_t drizzle::Result::warningCount() const {
    return drizzle_result_warning_count(this->result);
}

uint16_t drizzle::Result::columnCount() const {
    return this->totalColumns;
}

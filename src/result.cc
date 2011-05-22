// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./result.h"

node_db_drizzle::Result::Column::Column(drizzle_column_st *column) {
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

node_db_drizzle::Result::Column::~Column() {
}

std::string node_db_drizzle::Result::Column::getName() const {
    return this->name;
}

node_db::Result::Column::type_t node_db_drizzle::Result::Column::getType() const {
    return this->type;
}

node_db_drizzle::Result::Result(drizzle_st* drizzle, drizzle_result_st* result) throw(node_db::Exception&)
    : columns(NULL),
    totalColumns(0),
    rowNumber(0),
    empty(true),
    drizzle(drizzle),
    result(result),
    previousRow(NULL),
    nextRow(NULL) {
    if (this->result == NULL) {
        throw node_db::Exception("Invalid result");
    }

    if (drizzle_column_buffer(this->result) != DRIZZLE_RETURN_OK) {
        throw node_db::Exception("Could not buffer columns");
    }

    this->totalColumns = drizzle_result_column_count(this->result);
    if (this->totalColumns > 0) {
        this->empty = false;
        this->columns = new Column*[this->totalColumns];
        if (this->columns == NULL) {
            throw node_db::Exception("Could not allocate storage for columns");
        }

        uint16_t i = 0;
        drizzle_column_st *current;
        while ((current = drizzle_column_next(this->result)) != NULL) {
            this->columns[i++] = new Column(current);
        }

        this->nextRow = this->row();
    }
}

node_db_drizzle::Result::~Result() {
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

bool node_db_drizzle::Result::hasNext() const {
    return (this->nextRow != NULL);
}

char** node_db_drizzle::Result::next() throw(node_db::Exception&) {
    if (this->previousRow != NULL) {
        drizzle_row_free(this->result, this->previousRow);
    }

    if (this->nextRow == NULL) {
        return NULL;
    }

    this->rowNumber++;
    this->previousRow = this->nextRow;
    this->nextRow = this->row();

    return this->previousRow;
}

unsigned long* node_db_drizzle::Result::columnLengths() throw(node_db::Exception&) {
    return drizzle_row_field_sizes(this->result);
}

char** node_db_drizzle::Result::row() throw(node_db::Exception&) {
    drizzle_return_t result;
    char** row = drizzle_row_buffer(this->result, &result);
    if (result != DRIZZLE_RETURN_OK) {
        if (row != NULL) {
            drizzle_row_free(this->result, row);
            row = NULL;
        }
        throw node_db::Exception("Could not prefetch next row");
    }
    return row;
}

uint64_t node_db_drizzle::Result::index() const throw(std::out_of_range&) {
    if (this->rowNumber == 0) {
        throw std::out_of_range("Not standing on a row");
    }
    return (this->rowNumber - 1);
}

node_db_drizzle::Result::Column* node_db_drizzle::Result::column(uint16_t i) const throw(std::out_of_range&) {
    if (i < 0 || i >= this->totalColumns) {
        throw std::out_of_range("Wrong column index");
    }
    return this->columns[i];
}

uint64_t node_db_drizzle::Result::insertId() const {
    return drizzle_result_insert_id(this->result);
}

uint64_t node_db_drizzle::Result::affectedCount() const {
    return drizzle_result_affected_rows(this->result);
}

uint16_t node_db_drizzle::Result::warningCount() const {
    return drizzle_result_warning_count(this->result);
}

uint16_t node_db_drizzle::Result::columnCount() const {
    return this->totalColumns;
}

uint64_t node_db_drizzle::Result::count() const throw(node_db::Exception&) {
    if (!this->isBuffered()) {
        throw node_db::Exception("Result is not buffered");
    }
    return drizzle_result_row_count(this->result);
}

bool node_db_drizzle::Result::isBuffered() const throw() {
    return (this->result->options & DRIZZLE_RESULT_BUFFER_ROW);
}

bool node_db_drizzle::Result::isEmpty() const throw() {
    return this->empty;
}

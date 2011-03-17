// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./connection.h"

node_db_drizzle::Connection::Connection() :
    mysql(true),
    drizzle(NULL),
    connection(NULL)
{
    this->quoteString = '\'';
    this->quoteField = '`';
    this->quoteTable = '`';
}

node_db_drizzle::Connection::~Connection() {
    this->close();
    if (this->drizzle != NULL) {
        drizzle_free(this->drizzle);
    }
}

bool node_db_drizzle::Connection::isMysql() const {
    return this->mysql;
}

void node_db_drizzle::Connection::setMysql(bool mysql) {
    this->mysql = mysql;
}

void node_db_drizzle::Connection::open() throw(node_db::Exception&) {
    this->close();

    if (this->drizzle == NULL) {
        this->drizzle = drizzle_create(NULL);
        if (this->drizzle == NULL) {
            throw node_db::Exception("Cannot create drizzle structure");
        }

        drizzle_add_options(this->drizzle, DRIZZLE_NON_BLOCKING);
    }

    this->connection = drizzle_con_create(this->drizzle, NULL);
    if (this->connection == NULL) {
        throw node_db::Exception("Cannot create connection structure");
    }

    drizzle_con_set_tcp(this->connection, this->hostname.c_str(), this->port);
    drizzle_con_set_auth(this->connection, this->user.c_str(), this->password.c_str());
    drizzle_con_set_db(this->connection, this->database.c_str());
    if (this->mysql) {
        drizzle_con_add_options(this->connection, DRIZZLE_CON_MYSQL);
    }

    drizzle_return_t result;
    try {
        while (!this->opened) {
            result = drizzle_con_connect(this->connection);
            if (result == DRIZZLE_RETURN_OK) {
                this->opened = true;
                break;
            } else if (result != DRIZZLE_RETURN_IO_WAIT) {
                throw node_db::Exception(drizzle_con_error(this->connection));
            }

            if (drizzle_con_wait(this->drizzle) != DRIZZLE_RETURN_OK) {
                throw node_db::Exception("Could not wait for connection");
            }

            if (drizzle_con_ready(this->drizzle) == NULL) {
                throw node_db::Exception("Could not fetch connection");
            }
        }
    } catch(...) {
        if (this->connection != NULL) {
            drizzle_con_free(this->connection);
            this->opened = false;
            this->connection = NULL;
        }
        throw;
    }
}

void node_db_drizzle::Connection::close() {
    if (this->connection != NULL) {
        drizzle_con_close(this->connection);
        drizzle_con_free(this->connection);
        this->connection = NULL;
    }
    this->opened = false;
}

std::string node_db_drizzle::Connection::escape(const std::string& string) const throw(node_db::Exception&) {
    char* buffer = new char[string.length() * 2 + 1];
    if (buffer == NULL) {
        throw node_db::Exception("Can\'t create buffer to escape string");
    }

    drizzle_escape_string(buffer, string.c_str(), string.length());

    std::string escaped = buffer;
    delete [] buffer;
    return escaped;
}

std::string node_db_drizzle::Connection::version() const {
    std::string version;
    if (this->opened) {
        version = drizzle_con_server_version(this->connection);
    }
    return version;
}

node_db::Result* node_db_drizzle::Connection::query(const std::string& query) const throw(node_db::Exception&) {
    if (!this->opened) {
        throw node_db::Exception("Can't execute query without an opened connection");
    }

    drizzle_result_st *result = NULL;
    drizzle_return_t executed;
    try {
        while (true) {
            result = drizzle_query(this->connection, NULL, query.c_str(), query.length(), &executed);

            if (executed == DRIZZLE_RETURN_OK) {
                break;
            } else if (executed != DRIZZLE_RETURN_IO_WAIT) {
                if (executed == DRIZZLE_RETURN_LOST_CONNECTION) {
                    throw node_db::Exception("Lost connection while executing query");
                }
                throw node_db::Exception(drizzle_con_error(this->connection));
            }

            if (drizzle_con_wait(this->drizzle) != DRIZZLE_RETURN_OK) {
                throw node_db::Exception("Could not wait for connection");
            }

            if (drizzle_con_ready(this->drizzle) == NULL) {
                throw node_db::Exception("Could not fetch connection");
            }
        }
    } catch(...) {
        if (result != NULL) {
            drizzle_result_free(result);
        }
        throw;
    }

    if (result == NULL) {
        throw node_db::Exception("Could not fetch result of query");
    }

    return new node_db_drizzle::Result(this->drizzle, result);
}

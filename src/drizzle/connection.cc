#include "connection.h"

using namespace drizzle;

Connection::Connection():
    port(3306),
    mysql(true),
    opened(false),
    drizzle(NULL),
    connection(NULL)
{
}

Connection::~Connection() {
    this->close();
    if (this->drizzle != NULL) {
        drizzle_free(this->drizzle);
    }
}

std::string Connection::getHostname() const {
    return this->hostname;
}

void Connection::setHostname(const std::string& hostname) {
    this->hostname = hostname;
}

std::string Connection::getUser() const {
    return this->user;
}

void Connection::setUser(const std::string& user) {
    this->user = user;
}

std::string Connection::getPassword() const {
    return this->password;
}

void Connection::setPassword(const std::string& password) {
    this->password = password;
}

std::string Connection::getDatabase() const {
    return this->database;
}

void Connection::setDatabase(const std::string& database) {
    this->database = database;
}

uint32_t Connection::getPort() const {
    return this->port;
}

void Connection::setPort(uint32_t port) {
    this->port = port;
}

bool Connection::isMysql() const {
    return this->mysql;
}

void Connection::setMysql(bool mysql) {
    this->mysql = mysql;
}

bool Connection::isOpened() const {
    return this->opened;
}

void Connection::open() throw(Exception&) {
    this->close();

    if (this->drizzle == NULL) {
        this->drizzle = drizzle_create(NULL);
        if (this->drizzle == NULL) {
            throw Exception("Cannot create drizzle structure");
        }

        drizzle_add_options(this->drizzle, DRIZZLE_NON_BLOCKING);
    }

    this->connection = drizzle_con_create(this->drizzle, NULL);
    if (this->connection == NULL) {
        throw Exception("Cannot create connection structure");
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
                throw Exception(drizzle_con_error(this->connection));
            }

            if (drizzle_con_wait(this->drizzle) != DRIZZLE_RETURN_OK) {
                throw Exception("Could not wait for connection");
            }

            if (drizzle_con_ready(this->drizzle) == NULL) {
                throw Exception("Could not fetch connection");
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

void Connection::close() {
    if (this->connection != NULL) {
        drizzle_con_close(this->connection);
        drizzle_con_free(this->connection);
        this->connection = NULL;
    }
    this->opened = false;
}

std::string Connection::version() const {
    std::string version;
    if (this->opened) {
        version = drizzle_con_server_version(this->connection);
    }
    return version;
}

Result* Connection::query(const std::string& query) const throw(Exception&) {
    if (!this->opened) {
        throw Exception("Can't execute query without an opened connection");
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
                    throw Exception("Lost connection while executing query");
                }
                throw Exception(drizzle_con_error(this->connection));
            }

            if (drizzle_con_wait(this->drizzle) != DRIZZLE_RETURN_OK) {
                throw Exception("Could not wait for connection");
            }

            if (drizzle_con_ready(this->drizzle) == NULL) {
                throw Exception("Could not fetch connection");
            }
        }
    } catch(...) {
        if (result != NULL) {
            drizzle_result_free(result);
        }
        throw;
    }

    if (result == NULL) {
        throw Exception("Could not fetch result of query");
    }

    return new Result(this->drizzle, result);
}

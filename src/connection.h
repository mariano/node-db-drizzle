// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#ifndef SRC_CONNECTION_H_
#define SRC_CONNECTION_H_

#include <drizzle.h>
#include <drizzle_client.h>
#include <string>
#include "./node-db/connection.h"
#include "./result.h"

namespace node_db_drizzle {
class Connection : public node_db::Connection {
    public:
        Connection();
        ~Connection();
        bool isMysql() const;
        void setMysql(bool mysql);
        bool isAlive(bool ping=false) throw();
        void open() throw(node_db::Exception&);
        void close();
        std::string escape(const std::string& string) const throw(node_db::Exception&);
        std::string version() const;
        node_db::Result* query(const std::string& query) const throw(node_db::Exception&);

    protected:
        bool mysql;

    private:
        drizzle_st* drizzle;
        drizzle_con_st* connection;
};
}

#endif  // SRC_CONNECTION_H_

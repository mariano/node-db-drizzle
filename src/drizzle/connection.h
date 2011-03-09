// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#ifndef SRC_DRIZZLE_CONNECTION_H_
#define SRC_DRIZZLE_CONNECTION_H_

#include <libdrizzle/drizzle.h>
#include <libdrizzle/drizzle_client.h>
#include <string>
#include "./exception.h"
#include "./result.h"

namespace drizzle {
class Connection {
    public:
        Connection();
        ~Connection();
        std::string getHostname() const;
        void setHostname(const std::string& hostname);
        std::string getUser() const;
        void setUser(const std::string& user);
        std::string getPassword() const;
        void setPassword(const std::string& password);
        std::string getDatabase() const;
        void setDatabase(const std::string& database);
        uint32_t getPort() const;
        void setPort(uint32_t port);
        bool isMysql() const;
        void setMysql(bool mysql);
        bool isOpened() const;
        void open() throw(Exception&);
        void close();
        std::string escape(const std::string& string) const throw(Exception&);
        std::string version() const;
        Result* query(const std::string& query) const throw(Exception&);

    protected:
        std::string hostname;
        std::string user;
        std::string password;
        std::string database;
        uint32_t port;
        bool mysql;
        bool opened;

    private:
        drizzle_st* drizzle;
        drizzle_con_st* connection;
};
}

#endif  // SRC_DRIZZLE_CONNECTION_H_

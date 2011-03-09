// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#ifndef SRC_DRIZZLE_EXCEPTION_H_
#define SRC_DRIZZLE_EXCEPTION_H_

#include <exception>

namespace drizzle {
class Exception : public std::exception {
    public:
        explicit Exception(const char* message);
        const char* what() const throw();
    protected:
        const char* message;
};
}

#endif  // SRC_DRIZZLE_EXCEPTION_H_

#ifndef _DRIZZLE__EXCEPTION_H
#define _DRIZZLE__EXCEPTION_H

#include <exception>

namespace drizzle{
class Exception : public std::exception {
    public:
        Exception(const char* message);
        const char* what() const throw();
    protected:
        const char* message;
};
}

#endif

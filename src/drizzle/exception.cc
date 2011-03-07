#include "exception.h"

using namespace drizzle;

Exception::Exception(const char* message) : exception() {
    this->message = message;
}

const char* Exception::what() const throw() {
    return this->message;
}

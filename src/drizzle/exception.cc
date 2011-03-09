// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./exception.h"

drizzle::Exception::Exception(const char* message) : exception(),
    message(message) {
}

const char* drizzle::Exception::what() const throw() {
    return this->message;
}

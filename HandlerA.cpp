#include "HandlerA.h"
#include <iostream>

template <typename T>
T HandlerA<T>::processRequest(T amount, T decision) {
    if (decision == "REJECT") {
        return decision;
    }

    if (amount > "1000") {
        if (this->getNext() != nullptr) {
            return this->getNext()->processRequest(amount, decision);
        }
    }

    return decision;
}

template class HandlerA<std::string>;
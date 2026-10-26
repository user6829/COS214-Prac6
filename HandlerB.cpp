#include "HandlerB.h"
#include <iostream>

template <typename T>
T HandlerB<T>::processRequest(T amount, T decision) {
    if (decision == "REJECT") {
        return decision;
    }

    if (amount > "5000") {
        if (this->getNext() != nullptr) {
            return this->getNext()->processRequest(amount, decision);
        }
    }

    return decision;
}

template class HandlerB<std::string>;
template class HandlerB<int>;
template class HandlerB<double>;
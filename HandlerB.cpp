#include "HandlerB.h"
#include <iostream>

template <typename T>
T HandlerB<T>::processRequest(T amount, const std::string& decision) {
    if (decision == "REJECT") {
        return decision;
    }

    double Namount = 0.0;
    try { Namount = std::stod(amount); } catch (...) { Namount = 0.0; }
    if (Namount > 5000.0) {
        if (this->getNext() != nullptr) {
            return this->getNext()->processRequest(amount, decision);
        }
    }

    return decision;
}

template class HandlerB<std::string>;

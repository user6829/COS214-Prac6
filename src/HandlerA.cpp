#include "HandlerA.h"
#include <iostream>
#include <stdexcept>

template <typename T>
T HandlerA<T>::processRequest(T amount, const std::string&   decision) {
    if (decision == "REJECT") {
        return decision;
    }

    std::size_t parsed = 0;
    double Namount = 0.0;
    try {
        Namount = std::stod(amount, &parsed);
    } catch (const std::exception&) {
        throw std::invalid_argument("Approval amount is not numeric: " + amount);
    }
    if (parsed != amount.size())
        throw std::invalid_argument("Approval amount is not numeric: " + amount);

    if (Namount > 1000.0) {
        if (this->getNext() != nullptr) {
            return this->getNext()->processRequest(amount, decision);
        }
    }

    return decision;
}

template <typename T>
T HandlerA<T>::processRequest(T amount, const std::string& decision,
                              WorkflowContext& context,
                              const std::string& stepId)
{
    const std::string result = decision;
    return this->getNext()
        ? this->getNext()->processRequest(amount, result, context, stepId)
        : result;
}

template class HandlerA<std::string>;

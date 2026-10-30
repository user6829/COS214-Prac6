#include "ApprovalHandler.h"
#include <string>

template <typename T>
ApprovalHandler<T>::ApprovalHandler() : nextHandler(nullptr) {}

template <typename T>
ApprovalHandler<T>* ApprovalHandler<T>::setNext(ApprovalHandler<T>* handler)
{
    nextHandler = handler;
    return handler;
}

template <typename T>
ApprovalHandler<T>* ApprovalHandler<T>::getNext() const
{
    return nextHandler;
}

template class ApprovalHandler<std::string>;
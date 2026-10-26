#include "AppovalHandler.h"
#include <string>

template <typename T>
ApprovalHandler<T>* ApprovalHandler<T>::setNext(ApprovalHandler<T>*  handler){
    this->nextHandler = handler;
    return handler;
}

// Explicit template instantiations for supported types
template class ApprovalHandler<std::string>;
template class ApprovalHandler<int>;
template class ApprovalHandler<double>;
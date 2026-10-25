#include "AppovalHandler.h"
#include <string>

template <typename T>
ApprovalHandler<T>* ApprovalHandler<T>::setNext(ApprovalHandler<T>*  handler){
    this->nextHandler = handler;
    return handler;
}

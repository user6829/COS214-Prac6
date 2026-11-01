#ifndef APPROVAL_HANDLER_H
#define APPROVAL_HANDLER_H
#include <string>

/**
 * @brief Abstract base class representing a handler in the approval chain.
 * @tparam T Data type used for request amounts and decisions.
 */
template<typename T>
class ApprovalHandler {
private:
    /**
     * @brief Pointer to the next handler in the chain.
     */
    ApprovalHandler<T>* nextHandler;

public:
    /**
     * @brief Sets the next handler in the chain of responsibility.
     * @param handler Pointer to the next approval handler.
     * @return Pointer to the set approval handler.
     */
    ApprovalHandler<T>* setNext(ApprovalHandler<T>* handler);

    /**
     * @brief Gets the next handler in the chain.
     * @return Pointer to the next approval handler.
     */
    ApprovalHandler<T>* getNext() const {
        return nextHandler;
    }

    /**
     * @brief Pure virtual function to process an approval request.
     * @param amount The request amount to evaluate.
     * @param decision The decision state or context.
     * @return The resulting decision or processed amount.
     */
    virtual T processRequest(T amount, T decision) = 0;
};

#endif
#ifndef HANDLER_A_H
#define HANLDER_A_H
#include "AppovalHandler.h"
#include <string>

/**
 * @brief Concrete approval handler implementation A.
 * @tparam T Data type used for request amounts and decisions.
 */
template <typename T>
class HandlerA : public ApprovalHandler<T>{
    public:
        /**
         * @brief Processes an approval request specific to Handler A.
         * @param amount The request amount to evaluate.
         * @param decision The decision state or context.
         * @return The resulting decision or processed amount.
         */
        T processRequest(T amount,T decision);
};

#endif
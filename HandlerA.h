#ifndef HANDLER_A_H
#define HANDLER_A_H
#include "ApprovalHandler.h"
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
        T processRequest(T amount, const std::string& decision)override;
};

#endif
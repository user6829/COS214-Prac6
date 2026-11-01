/**
 * @file HandlerB.h
 * @brief Concrete escalation handler in the approval Chain of Responsibility.
 */
#ifndef HANDLER_B_H
#define HANDLER_B_H
#include "ApprovalHandler.h"
#include "WorkflowContext.h"
#include <string>

/**
 * @brief Concrete approval handler implementation B.
 * @tparam T Data type used for request amounts and decisions.
 */
template <typename T>
class HandlerB : public ApprovalHandler<T>
{
public:
    /**
     * @brief Processes an approval request specific to Handler B.
     * @param amount The request amount to evaluate.
     * @param decision The decision state or context.
     * @return The resulting decision or processed amount.
     */
    T processRequest(T amount, const std::string& decision) override;
    T processRequest(T amount, const std::string& decision,
                     WorkflowContext& context,
                     const std::string& stepId) override;
};
#endif

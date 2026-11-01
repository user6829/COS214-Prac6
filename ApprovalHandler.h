/**
 * @file ApprovalHandler.h
 * @brief Abstract handler interface for chaining approval checks or escalation steps.
 */
#ifndef APPROVAL_HANDLER_H
#define APPROVAL_HANDLER_H
#include <string>
#include "WorkflowContext.h"

/**
 * @brief Abstract base class representing a handler in the approval chain.
 * @tparam T Data type used for request amounts and decisions.
 */
template <typename T>
class ApprovalHandler
{
private:
    /**
     * @brief Pointer to the next handler in the chain.
     */
    ApprovalHandler<T> *nextHandler;

public:
    ApprovalHandler(const ApprovalHandler<T>&) = delete;
    ApprovalHandler<T>& operator=(const ApprovalHandler<T>&) = delete;
    ApprovalHandler();
    virtual ~ApprovalHandler() { delete nextHandler; }

    /**
     * @brief Sets the next handler in the chain of responsibility.
     * @param handler Pointer to the next approval handler.
     * @return Pointer to the set approval handler.
     */
    ApprovalHandler<T> *setNext(ApprovalHandler<T> *handler);

    /**
     * @brief Gets the next handler in the chain.
     * @return Pointer to the next approval handler.
     */
    ApprovalHandler<T> *getNext() const;

    /**
     * @brief Pure virtual function to process an approval request.
     * @param amount The request amount to evaluate.
     * @param decision The decision state or context.
     * @return The resulting decision or processed amount.
     */
    virtual T processRequest(T amount, const std::string &decision) = 0;

    /**
     * @brief Processes a request with instance context and work-item identity.
     * @param amount The request amount or serialized input.
     * @param decision The current decision.
     * @param context The instance-scoped workflow context.
     * @param stepId The work-item identifier.
     * @return The resulting decision.
     */
    virtual T processRequest(T amount, const std::string &decision,
                              WorkflowContext &context,
                              const std::string &stepId)
    {
        (void)context;
        (void)stepId;
        return processRequest(amount, decision);
    }

protected:
    /**
     * @brief Records one rule result in the instance context.
     */
    void recordRule(WorkflowContext &context, const std::string &stepId,
                    std::size_t number, const std::string &name, bool passed,
                    const std::string &compared, const std::string &reason)
    {
        const std::string prefix = "rule:" + stepId + ":" +
                                   std::to_string(number) + ":";
        context.setData(prefix + "name", name);
        context.setData(prefix + "passed", passed ? "true" : "false");
        context.setData(prefix + "compared", compared);
        context.setData(prefix + "reason", reason);
    }

    /**
     * @brief Records a rule and updates the count for the work item.
     */
    void appendRule(WorkflowContext &context, const std::string &stepId,
                    const std::string &name, bool passed,
                    const std::string &compared, const std::string &reason)
    {
        int count = 0;
        try { count = std::stoi(context.getData("rule:" + stepId + ":count")); }
        catch (...) {}
        ++count;
        recordRule(context, stepId, static_cast<std::size_t>(count), name,
                   passed, compared, reason);
        context.setData("rule:" + stepId + ":count", std::to_string(count));
    }
};

#endif
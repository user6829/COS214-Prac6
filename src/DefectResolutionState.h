/**
 * @file DefectResolutionState.h
 * @brief Concrete State representing a work item undergoing defect resolution.
 */
#ifndef DEFECTRESOLUTIONSTATE_H
#define DEFECTRESOLUTIONSTATE_H

#include "WorkItemState.h"

/**
 * @class DefectResolutionState
 * @brief Concrete State representing a work item undergoing defect resolution.
 */
class DefectResolutionState: public WorkItemState {
public:
    bool canApprove() const override { return true; }
    std::string getStateName() const override;
    void handle(LeafWorkItem* item) override;
};
#endif
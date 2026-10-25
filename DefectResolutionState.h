/**
 * @file DefectResolutionState.h
 * @brief The header file for the concrete state, Defect Resolution
 */
#ifndef DEFECTRESOLUTIONSTATE_H
#define DEFECTRESOLUTIONSTATE_H

#include "WorkItemState.h"

class DefectResolutionState: public WorkItemState {
    std::string getStateName() const override;
    void handle(LeafWorkItem* item) override;
};
#endif
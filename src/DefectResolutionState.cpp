#include "DefectResolutionState.h"

class LeafWorkItem;

std::string DefectResolutionState::getStateName() const {
    return "Defect Resolution";
}

void DefectResolutionState::handle(LeafWorkItem* item) {
    (void)item; // The current state reports behaviour without inspecting the item.
    std::cout << "Work item is undergoing a defect resolution.\n";
}
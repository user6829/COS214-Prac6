#include "DefectResolutionState.h"

class LeafWorkItem;

std::string DefectResolutionState::getStateName() const {
    return "Defect Resolution";
}

void DefectResolutionState::handle(LeafWorkItem* item) {
    std::cout << "Work item is undergoing a defect resolution.\n";
}
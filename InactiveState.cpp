#include "InactiveState.h"

class LeafWorkItem;

std::string InactiveState::getStateName() const {
    return "Inactive";
}

void InactiveState::handle(LeafWorkItem* item) {
    std::cout << "Work item is inactive.\n";
}
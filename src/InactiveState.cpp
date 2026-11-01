#include "InactiveState.h"

class LeafWorkItem;

std::string InactiveState::getStateName() const {
    return "Inactive";
}

void InactiveState::handle(LeafWorkItem* item) {
    (void)item; // The current state reports behaviour without inspecting the item.
    std::cout << "Work item is inactive.\n";
}
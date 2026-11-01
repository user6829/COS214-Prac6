#include "RejectedState.h"

class LeafWorkItem;

std::string RejectedState::getStateName() const {
    return "Rejected";
}

void RejectedState::handle(LeafWorkItem* item) {
    (void)item; // The current state reports behaviour without inspecting the item.
    std::cout << "Work item has been rejected.\n";
}
#include "CompletedState.h"

class LeafWorkItem;

std::string CompletedState::getStateName() const {
    return "Completed";
}

void CompletedState::handle(LeafWorkItem* item) {
    (void)item; // The current state reports behaviour without inspecting the item.
    std::cout << "Work item has been completed.\n";
}
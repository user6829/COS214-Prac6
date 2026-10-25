#include "CompletedState.h"

class LeafWorkItem;

std::string CompletedState::getStateName() const {
    return "Completed";
}

void CompletedState::handle(LeafWorkItem* item) {
    std::cout << "Work item has been completed.\n";
}
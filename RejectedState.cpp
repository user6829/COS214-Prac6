#include "RejectedState.h"

class LeafWorkItem;

std::string RejectedState::getStateName() const {
    return "Rejected";
}

void RejectedState::handle(LeafWorkItem* item) {
    std::cout << "Work item has been rejected.\n";
}
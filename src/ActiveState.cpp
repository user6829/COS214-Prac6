#include "ActiveState.h"


class LeafWorkItem;

std::string ActiveState::getStateName() const {
   return "Active";
}

void ActiveState::handle(LeafWorkItem* item) {
    (void)item; // The current state reports behaviour without inspecting the item.
    std::cout << "Work item is Active.\n" ; 
}
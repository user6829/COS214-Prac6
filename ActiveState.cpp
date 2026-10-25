#include "ActiveState.h"


class LeafWorkItem;

std::string ActiveState::getStateName() const {
   return "Active";
}

void ActiveState::handle(LeafWorkItem* item) {
    std::cout << "Work item is Active.\n" ; 
}
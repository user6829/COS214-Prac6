#include "TransactionalContract.h"
#include <iostream>

void TransactionalContract::print() {
    if (!render().empty()) {
        std::cout << render();
        return;
    }
    std::cout << "========================================\n";
    std::cout << "        TRANSACTIONAL CONTRACT\n";
    std::cout << "========================================\n";
    std::cout << "This document constitutes a binding\n";
    std::cout << "agreement between the undersigned parties.\n";
    std::cout << "Both parties agree to the terms, conditions,\n";
    std::cout << "and obligations set forth herein.\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Party A:   ______________________\n";
    std::cout << "Party B:   ______________________\n";
    std::cout << "Value:     ______________________\n";
    std::cout << "Term:      ______________________\n";
    std::cout << "Signature A: ____________________\n";
    std::cout << "Signature B: ____________________\n";
    std::cout << "========================================\n";
}
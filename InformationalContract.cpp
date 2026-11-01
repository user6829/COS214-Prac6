#include "InformationalContract.h"
#include <iostream>

void InformationalContract::print() {
    if (!render().empty()) {
        std::cout << render();
        return;
    }
    std::cout << "========================================\n";
    std::cout << "        INFORMATIONAL CONTRACT\n";
    std::cout << "========================================\n";
    std::cout << "This document is provided for information\n";
    std::cout << "purposes only. It is non-binding and does\n";
    std::cout << "not create any legal obligations between\n";
    std::cout << "the parties involved.\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Parties:   ______________________\n";
    std::cout << "Subject:   ______________________\n";
    std::cout << "Date:      ______________________\n";
    std::cout << "========================================\n";
}
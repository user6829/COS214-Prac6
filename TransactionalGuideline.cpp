#include "TransactionalGuideline.h"
#include <iostream>

void TransactionalGuideline::print() {
    std::cout << "========================================\n";
    std::cout << "       TRANSACTIONAL GUIDELINE\n";
    std::cout << "========================================\n";
    std::cout << "This guideline describes mandatory\n";
    std::cout << "procedures that must be followed when\n";
    std::cout << "executing the associated transaction.\n";
    std::cout << "Non-compliance may void the transaction.\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Procedure: ______________________\n";
    std::cout << "Authority: ______________________\n";
    std::cout << "Effective: ______________________\n";
    std::cout << "========================================\n";
}
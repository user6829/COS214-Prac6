#include "TransactionalFactory.h"
#include "TransactionalContract.h"
#include "TransactionalGuideline.h"
#include <stdexcept>

Document* TransactionalFactory::createInstance(const std::string& type) {
    if (type == "contract") {
        return new TransactionalContract();
    } else if (type == "guideline") {
        return new TransactionalGuideline();
    }
    throw std::invalid_argument("Unknown transactional document type: " + type);
}

std::string TransactionalFactory::getType() {
    return "transactional";
}
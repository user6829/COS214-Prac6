#include "TransactionalFactory.h"
#include "TransactionalContract.h"
#include "TransactionalGuideline.h"

Document* TransactionalFactory::createInstance(const std::string& type) {
    if (type == "contract") {
        return new TransactionalContract();
    } else {
        return new TransactionalGuideline();
    }
}

std::string TransactionalFactory::getType() {
    return "transactional";
}
#include "InformationalFactory.h"
#include "InformationalContract.h"
#include "InformationalGuideline.h"

Document* InformationalFactory::createInstance(const std::string& type) {
    if (type == "contract") {
        return new InformationalContract();
    } else {
        return new InformationalGuideline();
    }
}
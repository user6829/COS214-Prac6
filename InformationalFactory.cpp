#include "InformationalFactory.h"
#include "InformationalContract.h"
#include "InformationalGuideline.h"
#include <stdexcept>

Document* InformationalFactory::createInstance(const std::string& type) {
    if (type == "contract") {
        return new InformationalContract();
    } else if (type == "guideline") {
        return new InformationalGuideline();
    }
    throw std::invalid_argument("Unknown informational document type: " + type);
}
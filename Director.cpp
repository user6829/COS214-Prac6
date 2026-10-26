#include "Director.h"
#include "DocumentationBuilder.h"
#include "Document.h"

Document* Director::construct() {
    builder->buildPart();
    return builder->getResult();
}
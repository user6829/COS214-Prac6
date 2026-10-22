#ifndef WORKFLOW_MEMENTO_H
#define WORKFLOW_MEMENTO_H

#include <string>
#include <map>

class WorkflowMemento {
private:
    std::map<std::string, std::string>* instanceState;
    std::string timestamp;

public:
    std::map<std::string, std::string>* getState();
};

#endif
/**
 * @file WorkItemState.h
 * @brief This class is the abstract state class for a LeafWorkItem
 */
#ifndef WORKITEMSTATE_H
#define WORKITEMSTATE_H

#include <string>
#include <iostream>

class LeafWorkItem;

class WorkItemState {
    public:
    /**@brief The destructor  */
    virtual ~WorkItemState() = default;
    /**@brief This function retrieves the state of the LeafWorkItem as a string
     * @return Returns the state as a string 
     */
    virtual std::string getStateName() const = 0;
    /**@brief Handles an items current state
     * @param The item that has the state in question
     */
    virtual void handle(LeafWorkItem* item) = 0;
   
};



#endif
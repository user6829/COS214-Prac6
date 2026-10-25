#ifndef EVENTOBSERVER_H
#define EVENTOBSERVER_H

#include <string>
#include <iostream>
#include "SubjectWorkFlowInstance.h"

    /**
    * @brief Observer interface
    */

class EventObserver
{
public:
    /**
    * @brief Updates the connected observer
    * @param s Used to call the public getters to get the updated values
    */
    virtual void update(SubjectWorkFlowInstance& s) = 0;
    virtual ~EventObserver(){};
};

#endif
/**
 * @file UniversityWorkflowDirector.h
 * @brief Director for the undergraduate admissions workflow definition.
 */
#ifndef UNIVERSITY_WORKFLOW_DIRECTOR_H
#define UNIVERSITY_WORKFLOW_DIRECTOR_H

#include "Director.h"

class WorkflowDefinition;

/**
 * @class UniversityWorkflowDirector
 * @brief Builds the university workflow using the original Builder pattern.
 */
class UniversityWorkflowDirector : public Director
{
public:
    WorkflowDefinition* constructWorkflow() override;
};

#endif

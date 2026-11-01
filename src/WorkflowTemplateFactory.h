/** @file WorkflowTemplateFactory.h
 * @brief Generic workflow configuration and construction; execution remains in the facade.
 */
#ifndef WORKFLOW_TEMPLATE_FACTORY_H
#define WORKFLOW_TEMPLATE_FACTORY_H
#include <string>
#include <vector>
class WorkflowDefinition;
namespace GenericDriver {
enum class ExecutionMethod { Manual, Automated };
enum class WorkflowType { Case, Project, Approval, DocumentManagement };
enum class WorkflowStructure { RuleDriven, StateMachine, Parallel, Sequential };
enum class FieldType { Text, Number };

struct FieldSpec
{
    std::string name;
    FieldType type;
};

struct WorkflowDefinitionSpec
{
    std::string id;
    std::string industry;
    std::string name;
    ExecutionMethod method;
    WorkflowType type;
    WorkflowStructure structure;
    std::vector<std::string> roles;
    std::vector<FieldSpec> fields;
    std::string ruleField;
    double threshold{0.0};
    std::string belowPath;
    std::string abovePath;
    std::vector<std::string> sequence;
    /** Optional per-task stage and role configuration; empty retains templates. */
    std::vector<std::string> stageNames;
    std::vector<std::string> workItemRoles;
};


/** Creates configured Composite definitions using the original workflow Builder. */
class WorkflowTemplateFactory {
public:
    static std::vector<std::string> sequence(WorkflowType type, WorkflowStructure structure);
    /** Caller owns the returned definition until facade registration. */
    static WorkflowDefinition* createDefinition(const WorkflowDefinitionSpec& spec);
};
}
#endif

# Class diagram changes

This document records the source-level changes required for the class diagram.

## Generic engine

* `ApprovalHandler<T>` retains `setNext`, `getNext`, and the original pure
  `processRequest(T, const std::string&)`. It adds a context-aware overload and
  protected rule-recording helpers. `HandlerA<T>` and `HandlerB<T>` override the
  overload and preserve the existing two-argument behavior.
* `LeafWorkItem` calls the context-aware handler overload and stores its
  pre-approval result in the instance context.
* `TividyEngineFacade` recognizes `DEFECT_RESOLUTION`, blocks later work after
  that status, records failed-rule names in approval errors, and keeps the
  original snapshot/rollback API.
* `WorkflowBuilder` adds non-pure overloads for explicit work-item IDs and
  names, approval handlers, initial states, and definition metadata.
  `ConcreteWorkflowBuilder` owns pending routing, approval, and state objects
  until the next work item is built.
* `Director::constructWorkflow()` is virtual and `Director` has a virtual
  destructor. `UniversityWorkflowDirector` and
  `ProcurementWorkflowDirector` are concrete Builder-pattern Directors.
* `Document` adds instance data, `setData`, and `render`; existing
  `InformationalGuideline`, `TransactionalGuideline`,
  `InformationalContract`, and `TransactionalContract` render supplied
  content while retaining their original placeholder output when no content
  is supplied.
* `DocumentationBuilder` adds a default `setData` method.
  `ContractBuilder` and `GuidelineBuilder` accept data and can select the
  transactional or informational factory family.

## Industry-specific participants

* `UniversityRuleHandler` subclasses `ApprovalHandler<std::string>` and
  evaluates APS, English, required-subject, and capacity rules.
* `ProcurementRuleHandler` subclasses `ApprovalHandler<std::string>` and
  evaluates specification, supplier, authority, and funding rules.
* `ProcurementRules` remains the construction business-policy object.

## Existing diagram mismatches still present

The source uses `getStateName()` and `handle(LeafWorkItem*)`, not the diagram's
`InProgress`/`handleXState` names. `EventObserver` uses
`update(SubjectWorkFlowInstance&)`, not `onEvent`. `AuditCaretaker`,
`WorkflowMemento`, `WorkflowContext`, `RoutingStrategy`, and
`TividyEngineFacade` expose additional methods including `getAll`,
`replaceAll`, `describe`, and instance-specific snapshot access.
The source class is `Director`, not `WorkFlowDirector`.

The old diagram classes `NameFactory`, `MarksFactory`, `ApplicationComponent`,
`NameSection`, `MarksSection`, `FirstNames`, `LastName`, `Grade11Marks`, and
`Grade12Marks` do not exist in this workspace. They are represented by the
current document factories/products, workflow components, and
`UniversityApplication` data.

## Shared-engine integration refactor

The current architecture and exact manual diagram edits are documented in
[REFACTOR_REPORT.md, section H](REFACTOR_REPORT.md#h-exact-manual-uml-changes).
This supersedes older descriptions that place runtime State on definition leaves
or describe separate driver-owned engines. Main owns the single facade; instances
own runtime State objects; ProcurementRules realizes EventObserver; the extracted
WorkflowTemplateFactory constructs definitions through the existing Builder.
Builder/Abstract Factory document products are actually rendered and saved.

## Final architecture cleanup

The current implementation supersedes the earlier scenario-module layout.
See [FINAL_ARCHITECTURE.md](FINAL_ARCHITECTURE.md), especially section J, for
exact manual UML updates. Main owns the facade and BudgetLedger. FixedScenarios,
GenericWorkflowDriver and ProcurementRules source/header pairs are removed after
verification. ProcurementRuleHandler owns domain checks; BudgetLedger owns balances
and observes approved completion. Document assembly belongs to the existing
builders, and Director gates it through actual engine outcomes. AuditLoggerObserver
now stores readable history. The factory accepts explicit stage/task/role data;
automated progression remains in the original facade.

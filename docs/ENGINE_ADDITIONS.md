# Engine additions

| Addition | Original participant | Why the original participant could not do it | Size |
|---|---|---|---|
| Context-aware `ApprovalHandler<T>::processRequest` overload and rule-result recording helpers | `ApprovalHandler<T>` | The original two-argument API has no instance context or work-item identity | Small |
| Context-aware forwarding in `HandlerA` and `HandlerB` | Chain of Responsibility handlers | Existing forwarding only supports the legacy two-argument call | Small |
| `DEFECT_RESOLUTION` status handling and failed-rule approval refusal | `TividyEngineFacade` and `LeafWorkItem` | The original facade only represented pending, approved, rejected and waitlisted transitions | Small |
| `UniversityRuleHandler` | `ApprovalHandler<std::string>` | Admissions checks need access to instance data while remaining stateless across instances | Medium |
| `ProcurementRuleHandler` | `ApprovalHandler<std::string>` | Procurement checks need access to instance data while remaining stateless across instances | Medium |
| Builder overloads for explicit IDs, names, state, approval handler and metadata | `WorkflowBuilder` / `ConcreteWorkflowBuilder` | The original builder could only use one string for both IDs and display names and had no handler/state attachment API | Medium |
| `UniversityWorkflowDirector` | `Director` | The original director only builds its sample workflow | Small |
| `ProcurementWorkflowDirector` | `Director` | The original director only builds its sample workflow | Small |
| Document data/render support | `Document`, `DocumentationBuilder` and existing products | Placeholder products had no instance-specific payload or render operation | Medium |
| Transactional selection and data support in `ContractBuilder` / `GuidelineBuilder` | Existing documentation builders | Existing builders always selected the informational factory and accepted no data | Small |

The original two-argument handler API, original builder methods, original document
printing behavior, and Scenario 3 paths remain available.

## Shared-engine integration

See [REFACTOR_REPORT.md](REFACTOR_REPORT.md) for the current source-backed map.
The existing facade gained next-work selection, conditional/deferred task handling,
aggregate outcomes, terminal-document eligibility and event annotations. No second
engine was introduced. Existing State implementations gained permissions used by
the facade and are owned per instance. ProcurementRules now supplies chain checks
and observes successful completion for one-time budget commitment. The generic
factory was extracted and builds actual definitions through ConcreteWorkflowBuilder.

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

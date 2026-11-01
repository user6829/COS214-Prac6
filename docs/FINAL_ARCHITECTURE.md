# TIVIDY final architecture cleanup

## A. Architecture summary

`main.cpp` owns **one** original `TividyEngineFacade` and one session
`BudgetLedger`. The university and procurement Directors construct definitions
through the existing `ConcreteWorkflowBuilder`; the retained
`WorkflowTemplateFactory` constructs configurable definitions through that same
builder. All three options create independent instances through the same facade.

The engine selects available work, executes leaves, applies routing and approval,
changes per-instance State objects, publishes events and records mementos.
`runToCompletion` now accepts an optional progress callback: the engine owns the
automatic processing loop while main only presents before/after output. Manual
option 3 polls available work and submits the selected action to the facade.
Main contains no alternative prerequisite, branch or transition algorithm.

`main.cpp` groups its many preserved prompts/screens in private `FixedUI` and
`GenericUI` helper namespaces. Those are functions and presentation records, not
new classes or engines. Shared `UI::engine`/`UI::ledger` pointers borrow main's
objects. Instance-specific view records remain keyed by the actual engine ID.
The ledger outlives the facade, its definitions/handlers and all instances.

## B. Removed files

Deletion happened **after** the transferred code compiled, the engine/CLI suites
passed, and fixed-scenario output/document comparisons passed.

| Removed file | Responsibility transferred to |
|---|---|
| `FixedScenarios.cpp` | Forms, input validation, menu/action handlers, configuration and progress/output helpers in `main.cpp`; document assembly in existing builders; readable history in `AuditLoggerObserver`. |
| `FixedScenarios.h` | No module API or callbacks are needed; helpers are private to main's translation unit. |
| `GenericWorkflowDriver.cpp` | Generic wizard, selection, manual interaction, status/history and output helpers in `main.cpp`; automated execution in facade; document assembly in builders. |
| `GenericWorkflowDriver.h` | No separate UI module API is needed. |
| `ProcurementRules.cpp` | Supplier/authority/specification/funding decisions in existing `ProcurementRuleHandler`; balances and one-time commitment in `BudgetLedger`. |
| `ProcurementRules.h` | Budget API in `BudgetLedger.h`; check result in `ProcurementRuleHandler::CheckResult`; input-only `ProcurementData` record in main. |

The Makefile removes all three obsolete implementations and compiles
`BudgetLedger.cpp`. Obsolete generated `.o`/`.d` files were removed. Backup and
reference sources, archives and original example documents were not modified.

## C. main.cpp structure

The entry point is a small composition root: construct ledger/facade, validate
programme configuration, register fixed definitions, display launcher, dispatch
scenario menu and detach/release observers on exit.

Important helper groups:

- Application: `printLauncher`, `readLauncherChoice`, `runScenarioMenu`.
- Fixed input: `ask`, `askRequired`, `askIntRange`, `askChoice`, `askEmail`,
  `askPositiveAmount`, `askSubjects`, `askNationality`, `askUniversityProgramme`,
  `askDate`, `askQuantity`, student/request ID generation.
- University: `menuStartUniversity`, `menuEditUniversity`,
  `setUniversityApplicationContext`, `displayUniversityChecks`,
  `processUniversityApplication`, `showUniversityResult`,
  `menuInspectUniversity`, `menuUniversityHistory`, `menuDocuments`.
- Procurement: `collectProcurementInformation`, `menuStartProcurement`,
  `setProcurementContext`, `printProcurementCheck`, `displayProcurementResult`,
  `menuProcurementStatus`, `menuProcurementHistory`, `menuProcurementDocuments`.
- Progress/output: `presentStep`, `stageFor`, `processAvailableWork`,
  `sendAdapterAndVisitorSummary`, document save/display helpers.
- Generic configuration: `chooseMethod`, `chooseType`, `chooseStructure`,
  `collectFields`, `collectRoutingRule`, `collectDetails`, `printReview`,
  `configureStagesAndWorkItems`, `defineWorkflow`.
- Generic interaction: `runInstance`, `chooseAndRun`, `presentGenericTask`,
  `printProcessing`, `refreshRecord`, `presentGenericCompletion`, `viewDefinitions`,
  `viewStatus`, `viewHistory`, `generateDocument`.

The file remains substantial because all original forms/screens were explicitly
requested in main. It is divided into UI helpers, with business validation,
ledger ownership and document-formatting algorithms kept out of those helpers.
The document `build*` helpers are one-line calls to the existing Director API;
they do not build or format contents.

## D. Procurement responsibility mapping

| Responsibility | Class responsible | Pattern | Reason |
|---|---|---|---|
| Request/specification validation | `ProcurementRuleHandler("specification", ledger)` | Chain of Responsibility | Validate material presence, positive finite quantity/value, unit and calendar-valid delivery date using this instance's context. |
| Supplier eligibility | `ProcurementRuleHandler("supplier", ledger)` | Chain of Responsibility | Preserve approved supplier policy without putting it in the generic engine or UI. |
| Requester authority | `ProcurementRuleHandler("authority", ledger)` | Chain of Responsibility | Preserve Site manager R150,000, Quantity surveyor R300,000, Project manager R1,000,000. |
| Routing/escalation | `RoleBasedRouting`, `ValueThresholdRouting` | Strategy | Preserve role assignments and R100,000 quotation escalation; routing never commits or approves. |
| Live budget availability | Funding `ProcurementRuleHandler` queries `BudgetLedger::canFund` | Chain + data-owning ledger | Consult current shared balance for every request, never a stale input field. |
| Shared balances | `BudgetLedger` | Domain data owner | Own material account budget/committed data independently from request contexts. |
| Budget commitment | `BudgetLedger::update` | Observer | Require the real approved completion/finance decision; commit once per instance; reject inconsistent replay. No public raw commit method exists. |
| Work execution/outcomes | `LeafWorkItem`, `WorkflowInstance`, `TividyEngineFacade` | Composite, State, Facade | Select, validate and perform actual work; failed checks lead to real rejection and block later work. |
| Audit/history | `AuditLoggerObserver`, `WorkflowMemento`, `AuditCaretaker` | Observer, Memento | Preserve readable history and inspectable engine snapshots. |

The ledger contains **no** supplier policy, authority limits, form validation,
approval chain or workflow runner. Each procurement handler evaluates only its
own rule and forwards via the existing chain; it no longer reevaluates an entire
policy object for every link. Its legacy context-free call fails closed, because
a procurement decision without request context cannot validate eligibility.

Original budgets/initial commitments remain:
Concrete 1,000,000/100,000; Cement 600,000/100,000;
Steel 500,000/470,000; Bricks 700,000/100,000;
Aggregates 800,000/100,000 (ZAR). Unknown materials have no account and cannot be
funded. Rejected requests never spend. Repeated completion notifications do not
spend twice. Multiple requests consult the same updated ledger.

No reservations or concurrent scheduler existed. The application remains
single-threaded; live rechecks and immediate commitment maintain the shared
balance for this execution model. This is not a distributed financial ledger.

## E. University document generation

Main requests documents through:

- `Director::constructUniversityDocument(engine, instanceId, contract)`
- `Director::constructProcurementDocument(engine, instanceId)`
- `Director::constructResultDocument(engine, instanceId, metadata, contract, transactional)`

The Director checks the facade's terminal/document eligibility and takes input
from the actual engine context. University contracts additionally require an
`ACCEPTED` outcome and an admissions definition. Main cannot grant eligibility
by changing a presentation record.

The existing `ContractBuilder`/`GuidelineBuilder` implement `setWorkflowData` to
assemble the content. `DocumentationBuilder` supplies protected shared content
helpers. Contract assembly lives in `ContractBuilder.cpp`; letter/procurement/
result assembly lives in `GuidelineBuilder.cpp`. No new document class or dummy
factory product is introduced.

Builder assembles content and transfers ownership. Abstract Factory chooses the
informational/transactional family and allocates the actual concrete product.
Director coordinates these collaborators. Main only prints/renders the returned
product and saves it using the original paths and overwrite/stale-file behavior.
Acceptance letter, rejection reasons, contract terms/signature lines, attempts,
IDs, dates and purchase-order references are preserved.

## F. Generic option

Existing Manual/Automated, four workflow types and four structures remain.
Review option **7, Configure stages and work items**, now allows explicit stages,
ordered tasks and per-task roles. Template defaults remain available. The factory
builds real nested stages containing the configured leaves; it never executes.

The configuration record adds `stageNames` and `workItemRoles`, parallel to the
existing task `sequence`. Empty vectors retain the original template behavior.
The factory validates vector sizes and nonempty names. Rule-driven definitions
retain task 2 as gate and tasks 3/4 as mutually exclusive low/high alternatives;
the UI explains this convention. Condition metadata is applied to the actual
leaves even when those tasks share a custom stage.

Manual actions now differ meaningfully:

1. Execute calls `executeStep` and leaves approval-controlled work pending.
2. Approve submits an engine decision (executing first if necessary).
3. Reject records real engine rejection.
4. Waitlist records real engine waitlisting.
5. Stop leaves the instance available for Resume (menu option 9).

Automatic processing uses the same facade's `runToCompletion` and progress
callback. Resume continues the same actual instance, not a new definition run.
Completion/rejection, documents, snapshots, readable history, Adapter output and
Visitor reporting use engine outcomes. Repeated starts create independent IDs
and contexts. Custom labels and roles do not implement a separate execution path.

## G. Meaningful original GoF patterns

| Pattern | Category | Participants | Runtime use |
|---|---|---|---|
| Builder | Creational | WorkflowBuilder, ConcreteWorkflowBuilder, industry Directors; DocumentationBuilder, ContractBuilder, GuidelineBuilder, Director | Actual registered trees and actual rendered documents in all three options. |
| Abstract Factory | Creational | DocumentFactory, InformationalFactory, TransactionalFactory, four document products | Allocate the family of the document printed and saved. |
| Composite | Structural | WorkflowComponent, CompositeStage, LeafWorkItem | Nested stages/tasks, ordering and recursive traversal. |
| Facade | Structural | TividyEngineFacade | Sole workflow lifecycle/execution API. |
| Adapter | Structural | ExternalSystemConnector, LegacyiHookAdapter, LegacyiHookAPI | Convert completed payloads for the original console legacy integration. |
| Strategy | Behavioral | RoutingStrategy, RoleBasedRouting, ValueThresholdRouting | Select actual routed assignee, used by approval actor checks. |
| Chain of Responsibility | Behavioral | ApprovalHandler, HandlerA/B, UniversityRuleHandler, ProcurementRuleHandler | Real request validation/approval and rejection. |
| State | Behavioral | WorkItemState, concrete states, WorkflowInstance | Independent state objects gate valid execute/approve operations. |
| Observer | Behavioral | EventObserver, WorkflowInstance, NotificationObserver, StateObserver, AuditLoggerObserver, BudgetLedger | Notifications, stored readable history and approved budget commitment. |
| Memento | Behavioral | WorkflowInstance, WorkflowMemento, AuditCaretaker | Snapshots and admissions rollback/resubmission. |
| Visitor | Behavioral | WorkflowVisitor, MetricsReportVisitor, CompositeStage, LeafWorkItem | Count actual stages/tasks and report instance/pending metrics. |

Eleven patterns: two creational, three structural, six behavioral. The simple
workflow-template configuration factory is not counted as another Factory Method.
No unimplemented Command or extra pattern hierarchy was invented.

## H. Preservation of options 1 and 2

Before editing, `make test` passed 161 engine and 161 CLI checks. The pre-cleanup
executable was retained as a generated test reference. The expanded CLI suite can
run both versions with identical input in separate temporary directories:

```
TIVIDY_BASELINE="$PWD/tests/tividy_before_cleanup" python3 tests/run_scenarios.py
```

For the university/procurement cases, it compares complete console output after
normalizing clock times and compares every generated document byte-for-byte.
It checks acceptance/rejection, status/history, accepted-to-rejected resubmission,
capacity, stale document replacement, procurement approval/funding rejection,
authority and supplier failure, input retries, and shared scenario session IDs.
The original menus, prompts, stage/routing/state output and documents passed this
comparison before the obsolete files were removed.

A manifest records SHA-256 hashes of backup files, archives and original sample
documents. Those are checked for preservation; tests generate documents only in
isolated directories inside `tests/`.

## I. New classes/files

Only one new production class is needed: `BudgetLedger` in `BudgetLedger.h/.cpp`.
Existing WorkflowContext owns per-instance payload, ApprovalHandler owns request
validation, RoutingStrategy owns assignment, and AuditCaretaker owns snapshots.
None should own a shared procurement account ledger. This small domain data
owner is therefore justified and explicitly permitted by the task.

`ProcurementRuleHandler::CheckResult` relocates the old check-value record;
`FixedUI::ProcurementData` relocates the old input record. They introduce no new
process logic. `AuditLoggerObserver::Entry` takes over the previous private
UniversityHistoryObserver's history entry. That redundant private observer class
is removed; the original audit observer gains useful stored history.

No new template, manager/controller/service or engine was added. Factory and
existing document-builder sources were reused rather than creating new helpers
in extra production files.

## J. Exact UML updates

No editable UML file was found. Apply these manual changes:

1. Remove FixedScenarios, GenericWorkflowDriver and ProcurementRules as active
   file/module participants. UI helper namespaces belong to main, not class boxes.
2. Add `BudgetLedger : EventObserver`. Attributes:
   `accounts: map<string,Account>` and `commitments: map<string,Commitment>`;
   Account has budget/committed doubles, Commitment has material/string and
   amount/double. Methods: constructor, budgetFor, committedFor, remainingFor,
   canFund(material,amount), update(subject). No public commit operation.
3. Main owns ledger and facade. Facade owns definitions/instances/caretaker;
   instances own contexts and runtime State objects. Ledger outlives those
   borrowers. Instance-to-ledger association is non-owning Observer attachment.
4. `ProcurementWorkflowDirector` stores borrowed `BudgetLedger& ledger` and has
   required constructor `(BudgetLedger&)`. `ProcurementRuleHandler` likewise
   borrows `BudgetLedger&`, retains its rule string and inherits
   `ApprovalHandler<string>`. It adds private evaluate(context): CheckResult.
   Specification/supplier/authority/funding are configurations of this existing
   concrete class, not four unnecessary new subclasses.
5. `WorkflowDefinition` adds `completionReversible: bool = true`,
   setCompletionReversible(bool), isCompletionReversible(): bool. Procurement's
   Director configures false. Facade rollback consults this generic policy,
   not a procurement ledger key. No ledger association is added to the engine.
6. Facade runToCompletion gains optional `ProgressCallback`, a
   `function<void(LeafWorkItem&,bool)>`. Before/after callbacks are presentation
   hooks; the facade retains the execution loop. Existing one-argument calls work.
7. `AuditLoggerObserver` adds per-ID entries and lastState maps, nested Entry,
   record(id,state,note), getHistory(id). Its update still prints the original
   audit output and now stores coherent lifecycle history. It owns entries only,
   not subjects. Existing observer base/virtual destructor remain.
8. `DocumentationBuilder` adds virtual setWorkflowData(map) and protected shared
   universityContent/resultContent formatting helpers. Both concrete builders
   override setWorkflowData. Their original setData/buildPart/getResult APIs and
   owned pending products remain.
9. `Director` adds the three static engine-gated document methods in E. It depends
   on the facade for eligibility and uses existing builders/factories, returning
   caller-owned Document*. Add no replacement product or document service class.
10. WorkflowDefinitionSpec adds stageNames/workItemRoles vectors. The existing
    WorkflowTemplateFactory uses them to configure ConcreteWorkflowBuilder.
    Factory returns an owned definition transferred to the facade; it has no
    instance, State, execution or budget ownership.
11. Preserve all earlier Composite, State, Strategy, Chain, Observer, Memento,
    Visitor, Facade and Adapter relationships. No routing subclass was added.

## K. Build and test results

Executed after transferring responsibilities and deleting the obsolete files:

- `make clean` followed by `make -j4`: successful clean C++11 build using
  `-std=c++11 -Wall -Wextra`; no compiler warnings or errors.
- `make test`: **181 engine checks passed**, existing shell smoke checks passed,
  and **173 CLI checks passed**.
- `TIVIDY_BASELINE="$PWD/tests/tividy_before_cleanup" python3 tests/run_scenarios.py`:
  **193 checks passed**, including full fixed-scenario console comparison
  (clock times normalized) and byte-for-byte document comparison.
- AddressSanitizer/UndefinedBehaviorSanitizer builds of application and engine
  tests used `g++ -std=c++11 -Wall -Wextra -g -I.
  -fsanitize=address,undefined -fno-omit-frame-pointer` and the current Makefile
  source list. **181 engine and 173 CLI checks passed**, without sanitizer
  diagnostics; the sanitizer compiler log is empty.
- Verified **101 SHA-256 hashes** for backups, archives and original example
  documents; all unchanged. Confirmed all six obsolete source/header files absent
  and no references to them in active code or Makefile.

Coverage includes university acceptance/four independent rule failures,
resubmission/capacity, all document families, procurement authority/supplier/
funding/specification failure, live shared balances, one-time commitment,
invalid transitions, independent contexts/states, rollback, all 16 template
combinations, custom stages/roles, conditional tasks sharing a stage, separate
manual execute/approve, waitlist/resume, pause/resume and EOF.

Logs: `tests/cleanup-final-build.log`, `tests/cleanup-final-tests.log`,
`tests/cleanup-preservation.log`, `tests/cleanup-sanitizer-build.log`,
`tests/cleanup-sanitizer-engine.log`, `tests/cleanup-sanitizer-cli.log`.
The suite uses only C++11 and Python's standard library. No dependencies were
installed. No Git/GitHub or remote operations were performed. There are no
remaining build failures or failing tests.

## L. Remaining limits

- Logical parallel work still runs sequentially; no concurrency was claimed or
  introduced. State-machine templates are ordered task lifecycles, not an
  arbitrary graphical transition editor.
- Custom rule-driven workflows follow the documented two-alternative gate
  convention; they are not a general expression/dependency language.
- Budgets, seats, workflows and audit remain session-only. No existing budget
  reservation, persistence or compensation system was removed.
- Completed irreversible definitions reject rollback until compensation exists.
  Rejected procurement can still be rolled back; it has not spent funds.
- Legacy transmission and email confirmation remain original console demos.
- The public C++ context API is mutable; this is not an authentication boundary
  against an arbitrary C++ caller deliberately forging internal fields.
- Main is a large UI translation unit because all preserved forms were requested
  there. Namespace groups and focused helper functions keep its entry point
  small; actual engine, domain and document algorithms remain outside it.

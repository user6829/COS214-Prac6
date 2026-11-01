# TIVIDY integration refactor and behavior-preservation report

## A. Architecture summary

`main.cpp` owns one `TividyEngineFacade` for the whole application, registers the
university and procurement definitions, supplies instance-creation callbacks,
and orchestrates all three scenario menus. University and construction share
that engine with the generic driver, including its instance counter and audit
caretaker. Scenario modules borrow the engine; they do not construct another one.

The existing facade is the authority for available work, prerequisite order,
conditional-task eligibility, approval decisions, lifecycle transitions, terminal
outcomes and document eligibility. Main's fixed-scenario processing loop asks
`getNextWorkItem()` and calls `advanceWorkflow()` without listing scenario steps.
The generic UI likewise asks the facade for each next task.

`FixedScenarios.cpp` retains substantial reusable forms and presentation code,
programme configuration, session lookup records and document formatting. It is
not a second progression engine. Its records mirror engine outcomes. Business
checks are read from the approval-chain context rather than independently deciding
whether a workflow has completed. Keeping these forms outside main avoids moving
hundreds of lines of terminal interaction and formatting into the entry point.

## B. Files modified

Each `.h` and `.cpp` named below is an actual changed file unless stated otherwise.

| Files | Change |
|---|---|
| `main.cpp` | Own the shared facade, configure modules, register fixed definitions, create instances through a callback, own scenario menu loops, and drive facade-selected work. Reject invalid launcher numeric prefixes; exit on EOF. |
| `FixedScenarios.h`, `FixedScenarios.cpp` | Replace module-owned engine/menu loops with borrowed engine, configuration/action and presentation helpers. Preserve guided forms, menus, edits, status/history and document actions. Display engine rule results, restore detailed stage/check output, and gate documents on terminal engine outcome. |
| `GenericWorkflowDriver.h`, `GenericWorkflowDriver.cpp` | Borrow shared engine; expose menu/action helpers to main. Use extracted factory and facade-selected next work. Implement real reject/waitlist/resume, engine-backed status/history, real document products, directory creation and final-context adapter output. |
| `TividyEngineFacade.h`, `TividyEngineFacade.cpp` | Add next-work, advance, automatic-run, event annotation, outcome and document eligibility APIs. Validate definition trees; prevent initial input from supplying execution/commit keys. Own aggregate terminal transitions; keep invalid decisions from mutating state; enforce per-instance State permissions. Reject committed procurement rollback without compensation. |
| `WorkflowDefinition.h` | Add success-result configuration (`ACCEPTED`, `APPROVED`, default `COMPLETED`). |
| `WorkflowInstance.h`, `WorkflowInstance.cpp` | Own runtime State objects by step ID; reconstruct after rollback; provide coherent single-event publication and work-item transitions. Disable unsafe copying. |
| `LeafWorkItem.h`, `LeafWorkItem.cpp` | Keep definition policies separate from runtime State; add deferred-task and threshold-branch metadata. Reset rule counts before evaluation. Disable unsafe copying. |
| `WorkItemState.h`, `ActiveState.h`, `InactiveState.h`, `DefectResolutionState.h` | Add virtual execution/approval permissions, used by facade. Expose concrete permission overrides publicly. Completed/Rejected inherit denial defaults. |
| `ConcreteWorkflowBuilder.h`, `ConcreteWorkflowBuilder.cpp` | Correct nested container tracking, guard invalid construction and policy self-assignment, and disable unsafe copying. |
| `Director.cpp` | Build the sample workflow beneath one root instead of replacing its first root. |
| `UniversityWorkflowDirector.cpp` | Configure successful outcome as `ACCEPTED`. |
| `ProcurementWorkflowDirector.h`, `ProcurementWorkflowDirector.cpp` | Inject the session policy into handlers; use consistent definition ID; configure `APPROVED`; mark inspection deferred. |
| `ProcurementRuleHandler.h`, `ProcurementRuleHandler.cpp` | Delegate checks to the existing policy instead of duplicating supplier, authority and funding decisions. |
| `ProcurementRules.h`, `ProcurementRules.cpp` | Evaluate policy from context, validate numeric/calendar specification inputs and guard commitments. Implement existing Observer interface to commit once on real approved completion. Initialize request amount. |
| `ValueThresholdRouting.cpp` | Reject nonfinite routing values/thresholds. |
| `MetricsReportVisitor.h`, `MetricsReportVisitor.cpp` | Count stages/items through real visitor callbacks; count only unfinished instances as running. |
| `ExternalSystemConnector.h` | Add virtual destructor for safe polymorphic destruction. |
| `ApprovalHandler.h`, `AuditCaretaker.h`, `WorkflowMemento.h`, `ContractBuilder.h`, `GuidelineBuilder.h` | Disable copying of objects that own raw pointers; retain existing collaborations and ownership-transfer APIs. |
| `Makefile` | Include extracted factory, generate header dependencies, add integration/CLI test target and clean dependency files. |
| `Doxyfile` | Exclude historical reference driver from current API documentation. |
| `README.md` | Document current shared-engine behavior, tests and limits. |
| `docs/CLASS_DIAGRAM_CHANGES.md`, `docs/ENGINE_ADDITIONS.md` | Append implemented relationships and exact manual UML updates. |

## C. New classes and files

- `WorkflowTemplateFactory.h/.cpp`: extract configuration types and factory from
  the generic driver. `WorkflowTemplateFactory::sequence()` uses the relocated
  `WorkflowTemplate`/`TupleWorkflowTemplate` participants for the 16 existing
  label sequences. `createDefinition()` uses the original
  `ConcreteWorkflowBuilder` to produce the actual registered definition.
- `ExecutionMethod`, `WorkflowType`, `WorkflowStructure`, `FieldType`, `FieldSpec`
  and `WorkflowDefinitionSpec` move to that header. They are not new engines.
- `WorkflowTemplate` and `TupleWorkflowTemplate` remain implementation details
  in the factory `.cpp`; no new public template hierarchy is introduced.
- `tests/engine_tests.cpp`: dependency-free C++11 engine integration checks.
- `tests/run_scenarios.py`: standard-library CLI regressions, with temporary
  working directories inside `tests/` so sample documents are not overwritten.
- `docs/REFACTOR_REPORT.md`: this report.

Existing document factories cannot select workflow sequences: their products
are `Document`, not `WorkflowDefinition`. The existing workflow builder can
construct a definition but does not select type/structure templates. The
extracted factory performs that configuration responsibility and delegates
construction to the builder. It never executes a workflow.

No Command, new manager/controller/service, new engine or external dependency
was added. No backup source or archive was changed.

## D. Removed or relocated responsibilities

- Module-owned facades and scenario menu loops move to main's shared session.
- Fixed hand-written execution sequences and UI lifecycle mutations are replaced
  by engine next-work/advance calls. Fixed rule-result presentation retains its
  industry-specific vocabulary and layouts.
- Procurement driver no longer makes a second approval decision or calls budget
  commitment after independently evaluating checks. Its policy is invoked by
  the chain and receives an actual completion event.
- Generic local factory/template code moves into the dedicated factory files.
  The discarded intermediate builder tree is removed; its result is now the
  registered definition. Generic `nextStep` progression is removed.
- Unreachable old fixed manual execution/decision/inspection helpers and disabled
  banner code were removed. The pre-refactor `runScenario()` dispatch exposed
  none of those helpers for options 1 and 2. No reachable action was removed.
- Driver result records remain presentation/domain records; their statuses come
  from the facade rather than authorizing execution.

## E. University document generation

A successful final offer leads the facade to publish `WORKFLOW_COMPLETED`, set
`State=Completed` and `Result=ACCEPTED`. A failed rule leads through
`DefectResolution` and an actual rejected work item to `WORKFLOW_REJECTED`,
`State=Rejected`, `Result=REJECTED`. Both transitions are snapshotted and observed.

The existing Generate documents menu remains. `canGenerateDocuments()` checks
the terminal instance and required work-item statuses before document output.
`buildUniversityDocument()` also verifies that its application record agrees
with the engine outcome. Contracts additionally require `ACCEPTED`. Rollback
revokes eligibility until processing reaches a terminal outcome again.

The preserved collaboration is:

```
main -> facade execution -> terminal event/outcome -> document menu
     -> ContractBuilder/GuidelineBuilder -> Director
     -> InformationalFactory/TransactionalFactory -> concrete Document
     -> render/print -> existing document file output
```

Builder owns assembling the configured product, applying instance content and
transferring ownership. Abstract Factory selects a coherent informational or
transactional product family. The document actually printed/saved is that
factory-created product; there is no dummy product followed by a separate text
wrapper.

Accepted applicants receive an informational acceptance letter and transactional
contract; rejected applicants receive an informational rejection letter. Existing
filenames, latest-attempt content and stale-file replacement are preserved.
Generation remains user-requested rather than silently writing files on every
terminal event. The terminal event makes generation eligible.

## F. Procurement integration

1. Existing prompts collect requester, role, project, material, quantity/unit,
   supplier, amount, calendar-valid delivery date and notes. Main's callback
   creates an actual `construction-procurement` instance.
2. Main asks the facade for the next enabled work item. The definition contains
   Request and Specification, Supplier and Cost Review, and Delivery and
   Inspection as real nested Composite stages.
3. `proc-submit` and `proc-spec` execute through the facade. `proc-quote` routes
   through the original threshold Strategy at R100,000 and is approved through
   its original handler collaboration.
4. `proc-approve` invokes the original approval chain. Concrete procurement
   handlers call the shared `ProcurementRules` policy for specification,
   approved supplier, requester authority and live budget availability. Results
   are stored in the instance context and displayed by the original check UI.
5. Failed checks enter defect resolution, then rejection through the facade.
   Downstream work cannot execute and no budget is committed.
6. Successful required work publishes approved completion. `ProcurementRules`,
   attached through `EventObserver`, verifies the definition, result and approved
   finance item and commits once per instance. Repeat notifications cannot
   double-commit. Another instance evaluates the updated live balance.
7. Inspection remains visible and deferred until delivery, explicitly outside
   this request-approval process, as before. It is excluded from request
   completion and is not falsely marked inspected.
8. Observers, instance snapshots, readable history, status lookup, result
   documents, Adapter output and Visitor reporting remain available.

Existing suppliers, authority limits, R100,000 routing threshold and material
budgets/initial commitments are unchanged. The `Other` material/supplier prompts
remain; unknown material still has no configured budget and an unapproved
supplier still fails eligibility. Delivery date rules match the original CLI.

## G. Runtime design-pattern mapping

| Pattern | Participants | Files | Runtime responsibility | Scenarios |
|---|---|---|---|---|
| Builder — creational | WorkflowBuilder, ConcreteWorkflowBuilder, industry directors; DocumentationBuilder, ContractBuilder, GuidelineBuilder, Director | Matching participant headers/implementations; WorkflowTemplateFactory.cpp | Construct the actual workflow tree and configured document products | All three |
| Abstract Factory — creational | DocumentFactory, InformationalFactory, TransactionalFactory and four document products | Matching participant files | Create the document family actually rendered/saved | All three |
| Composite — structural | WorkflowComponent, CompositeStage, LeafWorkItem | Matching participant files | Real nested stages, leaf order and recursive visitor traversal | All three |
| Facade — structural | TividyEngineFacade | TividyEngineFacade.h/.cpp | Single execution, approval, outcome and audit API | All three |
| Adapter — structural | ExternalSystemConnector, LegacyiHookAdapter, LegacyiHookAPI | Matching participant files | Convert final instance payload for legacy-format output | All three |
| Strategy — behavioral | RoutingStrategy, RoleBasedRouting, ValueThresholdRouting | Matching participant files | Choose assignee from instance data and supply approval actor gate | All three |
| Chain of Responsibility — behavioral | ApprovalHandler, HandlerA, HandlerB, UniversityRuleHandler, ProcurementRuleHandler | Matching participant files | Forward approval requests and combine industry validation results | All three; domain checks in fixed scenarios |
| Observer — behavioral | WorkflowInstance, EventObserver, console/history observers, ProcurementRules | WorkflowInstance.*, observer files, FixedScenarios.cpp, ProcurementRules.* | Publish coherent events, collect readable history, commit approved budget | All three; budget observer in procurement |
| State — behavioral | WorkItemState and existing concrete states; WorkflowInstance | State files, WorkflowInstance.*, facade | Per-instance state permissions gate execution/approval; transitions create concrete states | All three |
| Memento — behavioral | WorkflowInstance, WorkflowMemento, AuditCaretaker | Matching participant files | Per-instance snapshots, rollback and admissions resubmission | All three audit; university resubmission |
| Visitor — behavioral | WorkflowVisitor, MetricsReportVisitor, CompositeStage, LeafWorkItem | Matching participant files | Traverse real definitions to count stages/tasks; combine with instance metrics | All three |

This is 11 distinct GoF patterns: two creational, three structural and six
behavioral. The extracted template factory is a simple configuration factory;
it is not counted as an additional Factory Method or Abstract Factory. No
unimplemented Command pattern is claimed. The exact designated pattern list
from the assessment specification was not supplied, so its membership must be
checked against this concrete mapping rather than invented.

## H. Exact manual UML changes

No editable UML diagram file was found. Apply these source-backed updates:

1. `main` owns one facade; both scenario modules borrow it. Replace their old
   standalone entry loops with `configure`, menu and `handleAction` functions.
2. `WorkflowDefinition` adds `successResult: string`, default COMPLETED,
   `setSuccessResult(string)` and `getSuccessResult(): string`.
3. `LeafWorkItem` retains owned initial-state/routing/handler policies, adds
   `deferred: bool`, `conditionKey: string`, `conditionThreshold: double`,
   `conditionHigh: bool`, `setDeferred`, `isDeferred`, `setCondition`, `isEnabled`.
4. `WorkflowInstance` owns
   `workItemStates: map<string, unique_ptr<WorkItemState>>`; add
   `publish(state,event)`, `getWorkItemState(stepId): WorkItemState&`,
   `transitionWorkItem(stepId,status)`. Restoring a memento clears runtime states
   for reconstruction. Observers remain borrowed, context remains owned.
5. `WorkItemState` adds virtual `canExecute(): bool` and `canApprove(): bool`.
   Active allows approval, Inactive allows execution, DefectResolution allows approval;
   Completed/Rejected inherit denial. Facade depends on these permissions.
6. `TividyEngineFacade` adds `getNextWorkItem`, `advanceWorkflow`,
   `runToCompletion`, `canGenerateDocuments`, `getOutcome`, `recordEvent` and
   private `updateOutcome`. Original public execution/approval APIs remain.
7. `ProcurementRules` realizes `EventObserver`; add `evaluate(const WorkflowContext&)`,
   `update(SubjectWorkFlowInstance&)` and `committedInstances: set<string>`.
   It owns budget records, not workflow progression.
8. `ProcurementWorkflowDirector` adds a borrowed `ProcurementRules* policy`,
   supplied through its constructor. Each `ProcurementRuleHandler` also borrows
   that policy. Handlers evaluate it; instance events notify it.
9. Add extracted `WorkflowTemplateFactory` with static
   `sequence(type,structure)` and `createDefinition(spec): WorkflowDefinition*`.
   It depends on existing ConcreteWorkflowBuilder and returns ownership to caller.
   Relocate configuration enums/records into its header. Existing WorkflowTemplate
   and TupleWorkflowTemplate are private factory implementation participants.
10. `MetricsReportVisitor` adds `stagesCount` and `workItemsCount`; visit callbacks
    now increment them. `ExternalSystemConnector` gains virtual destructor.
11. Mark copy construction/assignment deleted on the owning types listed in B.
    Preserve original document Builder -> Abstract Factory -> Document relations;
    no replacement document class is added.
12. Show nested stage composition correctly. Procurement inspection is a deferred
    leaf, not an executed completion prerequisite. Branch conditions are leaf
    metadata interpreted by the facade, not a second routing/execution engine.

## I. Build and test results

Executed successfully:

- `make -j4`: C++11 application compilation and link.
- `make test`: rebuild as needed, run the C++ engine integration executable,
  run `sh tests/run_golden.sh`, and run `python3 tests/run_scenarios.py`.
- Engine suite: **161 checks passed**, including all 16 type/structure
  combinations at both threshold sides, optional routing for every structure,
  four admissions failures, procurement approval/rejection, live funding,
  one-time commitment, invalid transitions, instance isolation and rollback.
- CLI suite: **161 checks passed**, including both fixed forms, all 16 generic
  combinations, numeric optional routing, accepted/rejected documents,
  admission capacity and resubmission, stale-file replacement, procurement
  retry validation, supplier/authority/funding failures, manual waitlist/resume,
  shared instance numbering and EOF exits.
- Existing shell smoke checks: **passed**.
- Sanitizer builds use `g++ -std=c++11 -Wall -Wextra -g -I.
  -fsanitize=address,undefined -fno-omit-frame-pointer` with the Makefile source
  list for the application and engine sources for the test executable.
  `tests/engine_tests_sanitized` and
  `TIVIDY_BINARY="$PWD/tests/tividy_sanitized" python3 tests/run_scenarios.py`
  run the same engine/CLI checks under AddressSanitizer and
  UndefinedBehaviorSanitizer: **161 engine checks and 161 CLI checks passed**,
  with no sanitizer diagnostics. The sanitizer build log is empty (no warnings).

Logs: `tests/final-test.log`, `tests/sanitizer-build.log`,
`tests/sanitizer-engine.log`. No third-party dependencies were installed.
No application build or test failures remain. The initial build's unused helper
warning was removed; final normal and sanitizer builds report no warnings.

CLI tests use isolated temporary output directories inside `tests/`. Byte
comparison against the untouched local `hhhh.zip` confirmed that backup sources
and the original sample documents were unchanged. Archives were read without
extraction or modification. No Git, GitHub or remote operations were used.

## J. Preservation, intentional fixes and remaining concerns

During the in-progress refactor, detailed processing presentation and university
check layouts were initially simplified. This was reported when clarified and
restored using the untouched local pre-refactor source. The final application
retains the original fixed menu options and labels, all guided input prompts,
programme/supplier/material choices, retries, five-step forms, stage/routing/
action/state detail, four business-check blocks and summaries, result screens,
status/history lookup, edit/resubmit/cancel, letter/contract outputs and
adapter/metrics output. Original success/rejection notification wording is
published as engine audit annotations after the real outcome.

Preservation is functional, not byte-for-byte reproduction of misleading traces:
nonapproval tasks are no longer claimed to have been approved; observer events
are coherent single notifications; terminal states represent the actual process.
Failed admissions checks now reach a real rejection rather than throwing at the
next prerequisite. Subject token whitespace no longer changes eligibility.

Limits retained or made explicit:

- Logical parallel tasks run sequentially; no concurrent scheduler was added.
- State-machine templates are engine-governed ordered task/state sequences, not
  an arbitrary editable transition graph.
- Threshold branching supports the existing two alternative template tasks;
  it is not a general expression language or dependency graph.
- Legacy transmission and confirmation email remain console demonstrations.
- Instances, audit, programme seats and budgets are session-only.
- Procurement rollback after budget commitment is denied until compensation is
  implemented, preventing snapshots from silently undoing the commit marker.
- University seat release/reallocation remains scenario business bookkeeping
  around engine resubmission. There is no general transaction/compensation API.
- Context remains a public mutable string map. The facade validates progression
  and filters initial execution keys, but it is not an authentication/security
  boundary against a C++ caller mutating context directly.
- Document files remain explicitly generated/regenerated by menu; stale files
  are removed on regeneration, following the original behavior.
- Tests use assertions/standard-library subprocesses, not an installed third-party
  test framework. Timing metrics remain untracked.

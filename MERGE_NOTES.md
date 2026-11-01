# TIVIDY driver merge notes

## Sources

- The root engine, `README.md`, `docs/`, and `Makefile` are the canonical
  p222-style workspace revision.
- The fixed university and construction driver was moved from the original
  root `main.cpp` into `FixedScenarios.cpp`.
- The generic driver was ported from `generic_driver_reference.cpp` into
  `GenericWorkflowDriver.cpp`. The reference file remains source material and
  is not compiled.
- `main.cpp` is now the three-option launcher.

## Changes

- Added `FixedScenarios.h` and `GenericWorkflowDriver.h` entry-point headers.
- Added separate persistent facades and observer collections through the
  namespaces `FixedScenarios` and `GenericDriver`.
- Definitions are seeded lazily once per module. Leaving a mode and returning
  to it preserves its instances and history.
- Added launcher dispatch for university, construction, and generic workflows.
- Updated the Makefile and README for the merged program.

## Generic-driver compatibility fixes

The current engine enforces ordered prerequisites and role-matched approvals.
The generic driver now:

- executes and approves every leaf that enters `PENDING_APPROVAL`, including
  the parallel secondary track;
- reads `assignee:<step-id>` from the instance context and supplies that role
  to `processApproval`;
- records a scenario name in the context and reads `context:...` keys from
  mementos when listing instances and history;
- treats end-of-file input as a clean driver exit rather than looping.

The engine itself was not changed.

## Files not present

No misspelled duplicate headers, `build.log`, prebuilt executable, or
`__MACOSX` directory was present. The reference archives were not used as
engine sources. `p666.zip` is a stray nested archive and is removed from the
workspace; `generic_driver_reference.cpp` is retained until the port is
verified.

## Known limitations

- The engine's `Parallel` structure is represented by a composite tree and is
  executed in deterministic depth-first order because the current engine
  enforces ordered prerequisites; the generic driver does not create actual
  concurrent execution.
- The reference generic driver seeds eight type/structure pairs rather than
  all sixteen possible pairs. All eight seeded pairs pass in both manual and
  automated modes; an unseeded pair follows the existing custom-definition
  prompt instead of being silently invented.
- Metrics still report that total execution time is not tracked.

## Prompt A - engine integration

- A complete pre-edit copy of all `.cpp`, `.h`, `Makefile`, and `README.md`
  files is stored under `backup/promptA/`.
- The original facade, workflow component, routing, approval, observer,
  memento, visitor, adapter, builder, director, factory, and document product
  classes remain the implementation participants.
- Prompt A adds context-aware approval-handler calls, rule-result context
  records, `DEFECT_RESOLUTION`, two concrete industry rule handlers, explicit
  builder metadata/IDs/states/handlers, and two concrete workflow directors.
- Existing fixed-scenario entry points remain in `FixedScenarios.cpp` in this
  increment; moving the full orchestration into `main.cpp` and extracting
  shared console I/O remains a separate migration step.

## University refinement

- The University menu is now limited to start, status, edit/resubmit, history,
  documents, launcher return, and exit. Construction retains its original
  action menu.
- University start is a guided five-step flow: registration, information
  collection, processing, result display, and completion.
- Programme rules are session-scoped in `FixedScenarios.cpp`: undergraduate
  BSc Computer Science requires APS 34 and Mathematics, with capacity 4 and a
  threshold of 3; BCom Accounting requires APS 30 and Mathematics, capacity 4
  and threshold 3; BA Media Studies requires APS 28, capacity 4 and threshold
  3; BEng Civil Engineering requires APS 36 and Mathematics, capacity 3 and
  threshold 2. Seats increase only after acceptance and the threshold is the
  acceptance limit.
- The University history observer records the required applicant-facing states
  `Inactive`, `Active`, `DefectResolution`, `Rejected`, and `Completed`.
  Engine work-item notifications remain available to the existing observers
  but are excluded from this concise application log.
- University acceptance documents use the transactional factory family for the
  contract and the informational factory family for letters; generated
  application-specific text is displayed and saved to `documents/`. Existing
  document and builder participants remain unchanged for the other drivers.
  Regenerating a document overwrites the existing file and reports that fact.

## Refinements 2

- Removed the fixed-scenario `Loading reusable workflow definitions` heading
  and `[ready]` lines from `seedDefinitions()` while retaining all three
  registration calls and lazy one-time seeding.
- Added a University-only quiet mode for the existing notification, state, and
  audit observers during guided processing. The observers remain attached and
  Construction/Generic retain their default verbose output.
- Replaced the technical University Step 3 console output with four aligned,
  applicant-facing check blocks and a passed-check summary. This was expanded
  by Refinements 3 into the detailed stage/routing/observer report.

## Refinements 3

- Modified `FixedScenarios.cpp`, `README.md`, `MERGE_NOTES.md`, and `main.cpp`.
  No new source files were needed and no design-pattern participant classes
  were removed.
- Removed the postgraduate definition, postgraduate programme data, prompts,
  and menu paths. The University scenario now registers and runs only
  `undergraduate-admissions`; the generic engine and its other workflow
  definitions remain unchanged.
- Undergraduate programme capacities are now at least 100. BA Media Studies
  uses capacity 100, threshold 95, and 94 initially occupied seats so one
  qualified application can be accepted before the next reaches capacity.
  Startup validation rejects invalid capacity, threshold, or seat data.
- University applications now retain all editable fields and an attempt
  counter. The edit menu validates fields using the original form validators,
  supports multiple changes, shows before/after values, and discards proposed
  changes unless resubmission is confirmed.
- Resubmission uses `TividyEngineFacade::rollbackInstance(instanceId, 0)`.
  Snapshot zero is the instance's initial workflow state, so the same
  definition, ordered prerequisites, routing strategies, approval handlers,
  and actor-role checks are executed again without changing the facade API.
  A previously accepted programme seat is released before re-evaluation, and
  a new seat is allocated only after the latest attempt passes.
- The original instance ID and student number are preserved. The
  `UniversityHistoryObserver` remains attached and receives explicit attempt
  and processing entries, so prior attempts and field-change resubmissions
  remain visible rather than being replaced.
- Document generation includes the latest attempt and removes stale
  acceptance, contract, or rejection files before writing the current result's
  documents.
- Step 3 now restores detailed engine-facing processing output: Intake,
  Academic Screening, and Offer Decision stages show readable work-item names
  and IDs, routing decisions, routed roles, actions, state transitions,
  observer/audit/notification messages, and the four applicant checks. The
  same output is used for resubmissions. Git history could not be inspected
  because this workspace is not a Git repository; the current engine
  implementation and prior refinement notes were used instead.

## Procurement refinement

- Added `ProcurementRules.h/.cpp` and included `ProcurementRules.cpp` in the
  Makefile. The policy module owns supplier approval, requester authority,
  escalation, material budgets, committed balances, and the four ordered
  business checks. The Construction console only displays the structured
  results.
- Construction now exposes only Start request, View status, View history,
  Generate procurement documents, launcher return, and exit. The old
  continue, manual decision, rollback, adapter-sync, and metrics menu entries
  were removed; the facade rollback API and generic driver remain available.
- The new guided flow registers a request reference, collects separate first
  name/surname fields, drives `proc-submit`, `proc-spec`, `proc-quote`, and
  `proc-approve` through the facade with the routed assignee, and leaves
  `proc-inspect` as `Inactive - awaiting delivery (outside this request's
  approval process)`. Inspection is intentionally not simulated because it
  happens after delivery.
- Procurement rules are: Site manager R150,000, Quantity surveyor R300,000,
  Project manager R1,000,000; approved suppliers are BuildPro Supplies,
  SA Materials, and Civils Direct; routing escalates at R100,000. Budgets are
  Concrete R1,000,000/R100,000 committed, Cement R600,000/R100,000, Steel
  R500,000/R470,000, Bricks R700,000/R100,000, and Aggregates
  R800,000/R100,000. Steel is pre-seeded so insufficient funds can be
  demonstrated. Approved requests commit funds; rejected requests do not.
- Construction history reuses `UniversityHistoryObserver` and the existing
  observer/memento-backed workflow instance. It records the exact visible
  states `Inactive`, `Active`, `DefectResolution`, `Rejected`, and
  `Completed`; status/history lookups accept either request reference or
  instance ID.
- Procurement letters use the existing informational Abstract Factory family
  to preserve the document-pattern participants. Approval letters include a
  purchase-order reference; rejection letters include all reasons. Before
  regeneration, stale approval/rejection files are removed.
- Adapter and Visitor execution is now reachable from both guided scenarios:
  Step 5 sends the finished workflow payload through `LegacyiHookAdapter` and
  prints a session summary produced by `MetricsReportVisitor`. The Generic
  driver continues to use its existing visitor path.

### Runtime design-pattern coverage

| Pattern | Runtime location |
|---|---|
| Composite | University and procurement definitions execute nested stages and leaf work items. |
| Adapter | Guided University and Construction Step 5 send the finished payload to the legacy finance/student API. |
| Facade | Both guided flows register, start, execute, approve, and inspect instances through `TividyEngineFacade`. |
| State | Work-item states and the five applicant/request history states are emitted during processing. |
| Strategy | Role-based and threshold-based routing select each assignee. |
| Observer | Notification, state, audit, and history observers receive workflow events. |
| Memento | The facade snapshots workflow execution; University resubmission uses rollback and Construction history remains backed by instance snapshots. |
| Visitor | Guided Step 5 calls `exportMetrics` and prints the visitor-produced session summary. |
| Chain of Responsibility | Quote and approval handlers process procurement approvals and escalation. |
| Builder | Existing workflow builder/director code remains compiled and used by the generic workflow definitions. |
| Abstract Factory | University and procurement documents instantiate the informational/transactional document families. |
## Scenario 3 dynamic workflow redesign

- Replaced the old seeded Generic driver menu with a guided definition wizard
  and the eight-option Generic Workflow Driver menu.
- Definitions now collect execution method, workflow type, workflow structure,
  industry, process name, configurable roles, typed data fields, and optional
  numeric routing rules.
- Added distinct sequences for all 16 type/structure combinations, generated
  sequential `WF-*` definition IDs, review-before-registration output, and
  duplicate process-name validation.
- Manual execution finds the next task and displays its readable name, ID,
  routed role, routing reason, and action prompt. Automated execution runs the
  same definition without human intervention after data entry.
- Added session-scoped instance status, cumulative lifecycle history, result
  generation, document regeneration, Adapter transmission, and Visitor
  reporting. Result documents use the existing informational factory for Case
  and Document Management and the transactional factory for Project and
  Approval.
- Parallel definitions are intentionally logical branches: the existing
  engine executes them sequentially, so the UI explains the join limitation.
- The public `TividyEngineFacade` API and existing pattern participant classes
  were not changed.

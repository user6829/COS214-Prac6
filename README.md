# TIVIDY — Reusable Workflow Management System

C++11 terminal application for university undergraduate admissions, construction
material procurement and configurable generic workflows.

## Build and run

```sh
make
./tividy
```

The Makefile uses `g++ -std=c++11 -Wall -Wextra` and tracks header dependencies.
On Windows with MinGW, the executable is `tividy.exe`.

## Interactive terminal interface

On an interactive terminal, `./tividy` opens the ncurses dashboard. Use arrows and
Enter (or original menu numbers), Tab for form buttons, and F2 for full scrollable
activity. Escape uses an existing Back/Cancel/Exit action; Ctrl-C/Ctrl-D closes.
Mouse clicks work where the terminal reports them. A 50x18 or larger terminal is
recommended; resizing is supported.

```sh
./tividy --plain    # original CLI, also selected automatically for redirected input
./tividy --tui      # explicitly require an interactive ncurses terminal
make test-tui      # real PTY workflow and keyboard/mouse/resize checks
```

The Mac SDK's existing ncurses library is linked with `-lncurses`; no dependency
was installed. The original engine, business rules and document classes are
unchanged. [UI preservation and verification report](docs/TERMINAL_UI.md).

## Application options

1. **University application** preserves the guided five-step form, student and
   instance IDs, programme/subject/English/APS/capacity checks, acceptance and
   rejection results, status/history, editing and resubmission, and acceptance,
   rejection and contract documents.
2. **Construction material procurement** preserves the guided request form,
   requester authority, approved supplier and material budget checks, routing,
   approval/rejection results, status/history and purchase-order/rejection
   letters. Inspection remains visible and deferred until delivery.
3. **Generic workflow driver** defines and runs Manual/Automated workflows for
   Case, Project, Approval and Document Management with Rule-driven, State
   machine, Parallel and Sequential structures. It supports all 16 type/structure
   combinations, real reject/waitlist decisions and resuming paused or waitlisted
   instances (option 9), plus definitions, status, audit history and documents.

Scenario Exit/Choose a different scenario returns to the launcher; launcher 0
exits the application. Data remains available when returning to a scenario in
one session. EOF exits cleanly.

Main owns one original `TividyEngineFacade` shared by all three options.
All scenario menus, forms and presentation helpers now live in `main.cpp`;
`FixedScenarios`, `GenericWorkflowDriver` and `ProcurementRules` files are removed.
Procurement checks live in the existing approval handler. A small session-owned
`BudgetLedger` stores shared balances and commits approved completion events.
Only the engine chooses the next task, checks prerequisites, processes approval,
publishes terminal outcomes and permits documents. Rule-driven alternatives are
actually skipped by the engine when their configured condition is false.

## Industry policy

Admissions require English level 4 and the selected programme's minimum APS and
required subject. Programme acceptance thresholds and occupied seats are
session-scoped. BA Media Studies begins one seat below its threshold to reproduce
the capacity rejection path. Resubmission keeps student/instance IDs, increases
attempt and restores the engine's initial snapshot before reevaluation.

Procurement authority limits are R150,000 for Site manager, R300,000 for Quantity
surveyor and R1,000,000 for Project manager. Approved suppliers are BuildPro
Supplies, SA Materials and Civils Direct. Routing escalates at R100,000. Material
budgets retain their existing initial commitments; Steel starts with R30,000
available. The approval chain evaluates supplier, authority, specification and live funding
checks. The ledger observes real approved completion and commits exactly once. Rejected requests do not
commit funds. Committed procurement cannot be rolled back without compensation.

## Documents and patterns

Document menu actions use the original Builder/Director and informational or
transactional Abstract Factory products. They print and save the actual product's
rendered content under `documents/`. University acceptance contracts require an
approved terminal engine outcome. Regeneration replaces the latest-attempt
files and removes stale outcome documents.

Runtime GoF patterns: Builder, Abstract Factory, Composite, Facade, Adapter,
Strategy, Chain of Responsibility, Observer, State, Memento and Visitor.
Runtime State objects belong to independent instances; definition policies are
shared without runtime mutation. Visitor traversal measures real stages/tasks.
The extracted workflow-template factory configures definitions through the
existing builder and is not an execution engine. Wizard review option 7 lets users
configure actual stages, work items and per-task roles. Manual Execute leaves work
pending until a separate decision. Automated execution stays inside the facade.

## Tests

```sh
make test
```

This runs dependency-free C++11 engine integration tests, the existing shell
smoke checks and Python standard-library end-to-end regressions. CLI regressions
use temporary directories inside `tests/`, preserving sample documents. No
external test framework or dependency installation is required.

For a separately instrumented application binary, the CLI suite accepts:

```sh
TIVIDY_BINARY="$PWD/tests/tividy_sanitized" python3 tests/run_scenarios.py
```

## Documentation and limitations

`make docs` uses an already installed Doxygen to write HTML under
`docs/doxygen/`. Historical backup and reference sources are excluded.

[Final architecture report](docs/FINAL_ARCHITECTURE.md) covers removed files,
responsibility mapping, preservation, patterns, tests and exact manual UML changes.
The [earlier refactor report](docs/REFACTOR_REPORT.md) is retained as historical context.
[Class-diagram changes](docs/CLASS_DIAGRAM_CHANGES.md) records diagram updates.

Logical parallel tasks execute sequentially; state-machine templates are ordered
named lifecycle tasks rather than an arbitrary transition graph. Legacy API
transmission and confirmation email remain console demonstrations. Workflow
instances, budgets, seats and audit history are not persisted across application
runs. Execution duration is not tracked. Existing output files are examples, not
an engine persistence format.

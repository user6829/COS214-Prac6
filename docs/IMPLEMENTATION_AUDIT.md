# TIVIDY implementation audit (source review)

This note records source-level findings for the current code snapshot. It is not a substitute for the assessed final report, UML exports, a framework-based test suite, or the team's GitHub evidence.

## Confirmed by local checks

- The project compiles with the Makefile's `-std=c++11 -Wall -Wextra` flags.
- A smoke run can start an undergraduate admissions instance and prints a generated student number and instance ID.
- A smoke run can start a construction procurement instance and prints its instance ID.
- The three definitions are registered through the same `TividyEngineFacade` API.
- The source contains identifiable participants for Composite, Adapter, Facade, State, Strategy, Observer, Memento, Visitor, Chain of Responsibility, Builder and Abstract Factory.

## Functional fixes applied in this revision

- Routing strategies now receive the active `WorkflowContext` explicitly. They no longer depend on a context pointer retained by a shared work-item definition node.
- The selected assignee is stored under `assignee:<work-item-id>`. `processApproval` requires the actor role to match the selected assignee.
- The facade enforces ordered prerequisites: earlier work items must be completed or approved before a later item can execute; a rejected prerequisite blocks progression.
- Work-item statuses are persisted in the instance context. Execution marks ordinary items `COMPLETED` and approval-controlled items `PENDING_APPROVAL`; decisions transition them to `APPROVED`, `REJECTED` or `WAITLISTED`.
- Memento history supports per-instance indexing, and rollback replaces the full payload from the selected snapshot instead of leaving newer keys behind.
- A duplicate stage/work-item ID in the admissions offer stage was corrected so the work item can be selected unambiguously.

## Remaining limitations and submission checks

- Routing supports assignee selection and role-gated approvals, but arbitrary conditional branches are not implemented. Prerequisites are inferred from depth-first workflow order rather than configured as a dependency graph.
- Concrete `WorkItemState` classes still report state-specific behaviour; durable lifecycle status is held in each instance's context by the facade.
- Builder reset/ownership needs a separate review.
- The project should still be tested with the required GoogleTest, Catch2 or doctest framework. A clean build and focused functional check are useful but are not a substitute for that test suite, Doxygen generation, the final report, UML exports or GitHub evidence.

## Checks performed for this revision

- Clean C++11 build with `-Wall -Wextra` completed successfully.
- A focused functional harness passed checks for prerequisite blocking, routing assignment, approval transition, instance isolation and rollback to an instance-specific snapshot.
- Doxygen generation and framework-based testing were not performed in this environment.

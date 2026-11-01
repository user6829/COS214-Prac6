# TIVIDY — Workflow Management System

## 1. Project Overview

TIVIDY is a C++11 terminal application for undergraduate admissions, construction material procurement, and configurable workflows. Its shared engine executes ordered tasks, routes work to roles, validates business rules, and processes approval decisions. Users submit data, review pending work, inspect status and audit history, and generate outcome documents through ncurses or a plain console interface. The system demonstrates reusable object-oriented design, with simulated external integrations and session-only workflow data.

[Project documentation (Google Docs)](https://docs.google.com/document/d/1In7B0Ht3gtKLNqgxOAmGi0ctbhWBU1weo1u4oHTiVGs/edit?usp=sharing)

## 2. Design Patterns

| Pattern | Classes | Purpose |
|---|---|---|
| **Facade** | `TividyEngineFacade` | Provides a shared interface for workflow execution and decisions. |
| **Composite** | `WorkflowComponent`, `CompositeStage`, `LeafWorkItem` | Organizes workflows into nested stages and tasks. |
| **Builder** | `WorkflowBuilder`, `ConcreteWorkflowBuilder`, `Director`,`DocumentationBuilder`, `ContractBuilder`, `GuidelineBuilder` | Constructs workflow definitions and documents step by step. |
| **Abstract Factory** | `DocumentFactory`, `InformationalFactory`, `TransactionalFactory` | Creates informational and transactional contract/guideline families. |
| **Strategy** | `RoutingStrategy`, `RoleBasedRouting`, `ValueThresholdRouting` | Selects assignees using interchangeable routing policies. |
| **Chain of Responsibility** | `ApprovalHandler`, `HandlerA`, `HandlerB`, | Chains approval processing and business-rule checks. |
| **State** | `WorkItemState`, `InactiveState`, `ActiveState`, `CompletedState`, `RejectedState`, `DefectResolutionState`, `WorkflowInstance` | Governs task behavior and execution/approval permissions. |
| **Observer** | `SubjectWorkFlowInstance`, `WorkflowInstance`, `EventObserver`, `NotificationObserver`, `StateObserver`, `AuditLoggerObserver`, `BudgetLedger` | Responds to workflow changes with notifications, history, and budget commitments. |
| **Memento** | `WorkflowInstance`, `WorkflowMemento`, `AuditCaretaker` | Saves snapshots for audit history and rollback. |
| **Visitor** | `WorkflowVisitor`, `MetricsReportVisitor`, `CompositeStage`, `LeafWorkItem` | Collects workflow metrics through component traversal. |
| **Adapter** | `ExternalSystemConnector`, `LegacyiHookAdapter`, `LegacyiHookAPI` | Converts payloads for the legacy API demonstration. |

## 3. Requirements

- G++ or Clang++ with **C++11** support.
- **Make**.
- **ncurses** development headers and libraries; the Makefile links with `-lncurses`.

**macOS (Homebrew):**

```bash
brew install ncurses
```

**Ubuntu/Linux:**

```bash
sudo apt update
sudo apt install build-essential libncurses-dev
```

**Windows:** Use Ubuntu under WSL2 and run the same Ubuntu installation commands inside its shell.

## 4. Building and Running

Open a terminal in the project's root directory, where `Makefile` is located; on Windows, use the WSL Ubuntu terminal.

**Build the project:**

```bash
make
```

Compiles the application and creates the `tividy` executable.

**Build and run the project:**

```bash
make run
```

Builds the application if needed and launches it.

**Remove generated build files:**

```bash
make clean
```

Removes object files and the application/test binaries named in the Makefile.

If macOS cannot locate Homebrew's ncurses, supply its paths to the existing Makefile:

```bash
make CXXFLAGS="-std=c++11 -Wall -Wextra -g -I. -I$(brew --prefix ncurses)/include -I$(brew --prefix ncurses)/include/ncurses" \
  LDFLAGS="-L$(brew --prefix ncurses)/lib -Wl,-rpath,$(brew --prefix ncurses)/lib -lncurses"
```

## 5. Controls

Choose a scenario, complete its forms, and use the menus to review work, inspect history, or generate documents. Use arrow keys and Enter to select, Tab/Shift-Tab to move through forms, and Left/Right to change choice fields. F2 opens scrollable activity; Escape returns or cancels where available.

Return to the launcher and select **0** to exit. Use a terminal at least **50 columns × 18 rows**, or run `./tividy --plain` for line-oriented input.

Generated text documents are saved under `documents/` relative to the working directory; regenerating them can overwrite existing files.

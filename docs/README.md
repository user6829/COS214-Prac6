TIVIDY — Workflow Management System
1. Project Overview
TIVIDY is a C++11 terminal application for undergraduate admissions, construction material procurement, and configurable workflows. A shared engine executes ordered tasks, routes work to roles, checks business rules, and processes approval decisions. Users submit data, review pending work, inspect status and audit history, and generate outcome documents through an ncurses interface or plain console mode. The system demonstrates reusable object-oriented design; policies and external integrations are simulations, and workflow data lasts only for the current session.
2. Design Patterns
Pattern	Implementing classes	Purpose
Facade	`TividyEngineFacade`	Provides one interface for registration, execution, decisions, outcomes, and rollback.
Composite	`WorkflowComponent`, `CompositeStage`, `LeafWorkItem`	Represents workflows as nested stages and individual tasks.
Builder	`WorkflowBuilder`, `ConcreteWorkflowBuilder`, `Director`, `DocumentationBuilder`, `ContractBuilder`, `GuidelineBuilder`	Assembles workflow definitions and documents step by step.
Abstract Factory	`DocumentFactory`, `InformationalFactory`, `TransactionalFactory`; `InformationalContract`, `InformationalGuideline`, `TransactionalContract`, `TransactionalGuideline`	Creates related document products in informational or transactional families.
Strategy	`RoutingStrategy`, `RoleBasedRouting`, `ValueThresholdRouting`	Selects assignees using interchangeable role or numeric-threshold policies.
Chain of Responsibility	`ApprovalHandler`, `HandlerA`, `HandlerB`,	Chains approval processing and domain-specific checks.
State	`WorkItemState`, `InactiveState`, `ActiveState`, `CompletedState`, `RejectedState`, `DefectResolutionState`	Controls task behavior and execution/approval permissions through instance-owned states.
Observer	`SubjectWorkFlowInstance`, `WorkflowInstance`, `EventObserver`, `NotificationObserver`, `StateObserver`, `AuditLoggerObserver`, `BudgetLedger`	Reacts to workflow changes with notifications, history, and approved budget commitments.
Memento	`WorkflowInstance`, `WorkflowMemento`, `AuditCaretaker`	Captures state and context snapshots for history and rollback.
Visitor	`WorkflowVisitor`, `MetricsReportVisitor`, `CompositeStage`, `LeafWorkItem`	Collects stage/task metrics without embedding reporting in each component.
Adapter	`ExternalSystemConnector`, `LegacyiHookAdapter`, `LegacyiHookAPI`	Converts workflow payloads for the legacy API's console transmission demonstration.
3. Requirements
GCC (`g++`) or Clang with C++11 support; macOS `g++` may invoke Apple Clang.
GNU Make and a POSIX shell.
ncurses development headers and libraries; linking requires `-lncurses` even in plain mode.
macOS, Linux, or Windows through WSL2 with Ubuntu. The UI uses POSIX terminal APIs.
4. Installation and Compilation
Run build commands from the project directory containing `Makefile`. Clean first to replace any supplied objects or binaries with a local build. The actual Makefile compiles the application with `-std=c++11 -Wall -Wextra -g -I.` and links ncurses.
macOS
Install Apple's Command Line Tools if needed:
```bash
xcode-select --install
```
With Homebrew installed, install ncurses and build using its header and library paths:
```bash
brew install ncurses pkg-config
make clean
make -j4 \
  CXXFLAGS="-std=c++11 -Wall -Wextra -g -I. -I$(brew --prefix ncurses)/include -I$(brew --prefix ncurses)/include/ncurses" \
  LDFLAGS="-L$(brew --prefix ncurses)/lib -Wl,-rpath,$(brew --prefix ncurses)/lib -lncurses"
```
`brew --prefix ncurses` handles Intel and Apple Silicon installations. `-I` selects headers, `-L` selects libraries, and the runtime search path locates Homebrew's library when launching.
Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential libncurses-dev pkg-config
make clean
make -j4
```
The default Makefile's `-lncurses` uses the system library paths.
Windows (WSL2 with Ubuntu)
In an administrator PowerShell window:
```powershell
wsl --install -d Ubuntu
```
Restart if prompted, open Ubuntu, and complete its first-run user setup. Check `wsl --list --verbose` in PowerShell; if Ubuntu uses version 1, run `wsl --set-version Ubuntu 2`.
In the Ubuntu shell, change to the project directory and run:
```bash
sudo apt update
sudo apt install build-essential libncurses-dev pkg-config
make clean
make -j4
```
Build and run inside WSL; the resulting executable is `tividy`.
The existing Makefile build and manual compilation were verified on macOS with Apple Clang 21 and system ncurses. The bundled `make unit-test` suite passed 20 cases and 43 assertions. Homebrew-specific, Linux, and WSL2 builds have not been tested here.
5. Running the Program
On macOS, Linux, or inside WSL Ubuntu:
```bash
./tividy
```
Choose university admissions, procurement, or the generic workflow builder. Use arrow keys and Enter for menus, Tab/Shift-Tab to move through forms, and Left/Right to change choice fields. F2 opens scrollable activity; Escape returns or cancels where available. Select the acting role shown by routing when reviewing pending work.
Return to the launcher and select 0 to exit. Use a terminal at least 50 columns × 18 rows. For line-oriented input or redirected streams:
```bash
./tividy --plain
```
Generated text documents are saved under `documents/` relative to the working directory; regenerating them can overwrite existing files.

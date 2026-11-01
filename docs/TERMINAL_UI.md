# TIVIDY terminal UI upgrade

## A. Files changed

- `main.cpp`: route user input through the presentation adapter, set screen titles,
  supply dashboard/manual-action buttons, show scrollable results/errors, and
  select TUI or plain mode. All existing validators and workflow calls remain.
- `TerminalUI.h`, `TerminalUI.cpp` (added): C++11 ncurses session, menus, form
  fields, confirmations, output panels, scrolling and terminal restoration.
  These files include no workflow headers and make no engine calls.
- `Makefile`: add `TerminalUI.cpp`, link the available system `-lncurses`, exclude
  UI objects from engine tests and add `make test-tui`. No `.d` files are generated.
- `tests/run_scenarios.py`: retain existing tests, add cancellation/retry and
  remaining programme checks, and optionally replay them through the real TUI.
- `tests/tui_driver.py` (added): Python-standard-library PTY driver; sends actual
  keystrokes and checks terminal modes after exit.
- `tests/test_tui_controls.py` (added): keyboard, mouse, scrolling, resize,
  confirmation, cancellation, signal and plain-mode checks.
- `README.md`: launch/test instructions. This report documents the preservation
  checklist, verification and terminal limitations.

Generated baseline manifests, executables and logs are test artifacts. The
pre-UI executable was obtained read-only from the local pre-UI archive; that
archive was not extracted into or written over project sources.

## B. UI improvements and usage

Run `./tividy` from an interactive terminal for the ncurses interface. It has a
consistent black background, blue borders/selections, cyan headings, green
success and red rejection/error feedback. Dashboard choices are University
Application, Construction Material Procurement, Generic Workflow Builder and Exit.

Controls:

- Menus: Up/Down, Home/End, Page Up/Down, Tab and Enter. Original numeric choices
  still work, including multi-digit Cancel. Mouse clicks activate real options.
- Forms: editable fields with Left/Right, Home/End, Backspace/Delete and Ctrl-U.
  Tab selects Submit, History or Clear. Mouse Submit works. Numeric fields still
  pass their text to the original validation functions.
- Confirmations: Yes/No buttons, original y/n input, arrows and Enter. Default is
  No; Escape selects No. This returns the existing answer to the original branch.
- Escape returns through an existing Choose a different scenario, Cancel, Stop or
  Exit action. Required selection menus cannot silently cancel an existing form.
  Escape in a text field clears the current edit rather than inventing workflow
  cancellation.
- F2 opens the complete session activity viewer. Results/documents/history also
  appear in a scrollable panel with Continue. Use arrows, Page Up/Down, Home/End,
  Enter/Escape and mouse scrolling where reported by the terminal.
- Live output includes an activity indicator and the existing five-step progress.
  Observer, state, approval, audit, metrics, adapter and document messages are
  retained; they are not silently discarded.
- Ctrl-D/Ctrl-C closes cleanly. SIGINT/SIGTERM are handled through the UI loop.
  RAII restores stream buffers, mouse reporting, screen and saved terminal modes.

All `cout`/`cerr` messages are captured into the UI while ncurses is active. No
active TUI path reads directly from `cin` or writes workflow text onto the screen
behind ncurses. The original engine's console messages require no class changes.

Use `./tividy --plain`, or `TIVIDY_PLAIN=1 ./tividy`, for the original interaction.
Redirected input/output automatically uses plain mode. `--tui` explicitly requires
an interactive terminal with TERM. `--help` explains the flags.

## C. Behavior-preservation checklist / comparison

The interaction map was inspected before edits and recorded in
`tests/tui-preservation.json` (prompt inventory and protected-file hashes).
The following actions retain their original implementation and sequence:

| Area | Before and after | Verification |
|---|---|---|
| Launcher | University, procurement, generic and Exit | Plain comparisons and actual arrows/mouse/Enter through a PTY. |
| University menu | Start; status; edit/resubmit; history; documents; different scenario; Exit | Full PTY replay, including Cancel and declined resubmission. |
| University form | First name, surname, four programmes, email, citizenship, APS, English level, matric year, seven distinct selected NSC subjects | All fields replayed; all four programmes exercised. Required fields, email, range/year, duplicate/incompatible subject retries exercised. |
| University decisions | APS/English/subject/capacity checks; accepted/rejected; immediate edit/retry; unchanged IDs/incremented attempt | Engine tests, PTY acceptance/rejection, capacity, accepted-to-rejected and rejected-to-accepted resubmissions. |
| University documents | Acceptance letter, rejection letter, contract; current attempt; stale replacement | All generated filenames and bytes compared with the pre-UI executable. |
| Procurement menu | Start; status; history; documents; different scenario; Exit | Full PTY replay and mouse dispatch. |
| Procurement form | First/surname, requester role, project, material/Other, quantity/unit, supplier/Other, cost, date, notes | Full form; required/range/positive quantity/cost/calendar retry checks; supplier/authority/funding exceptions. |
| Procurement policy | Same suppliers, authority limits, value routing, material budgets and commitments | Engine/policy files hash-identical; real requests on both routing sides, funding rejection and sequential spending checked. |
| Procurement outcomes | Approved/rejected, no spending after rejection, one-time approved commitment, deferred inspection | Engine checks and repeated Steel requests through the TUI. |
| Generic menus | Define/run/list/status/history/documents/return/Exit/resume | All available through the original action handlers. |
| Generic configuration | Manual/automated; four types; four structures; industry/name; roles; typed fields; optional numeric rule; preview/edit/cancel; custom stages/tasks/roles | All 16 combinations, optional routing, custom stages and shared-stage conditional tasks replayed through the TUI. |
| Generic actions | Execute, Approve, Reject, Waitlist, Stop, Resume | Actual engine decisions and separate pending execute/approve verified. |
| Progress/history/output | State transitions, audits, snapshots, checks/results, metrics, adapter messages and document output | Logical transcript comparison plus full activity/result viewers; scrolling exercised. |
| Navigation/errors | Return/repeat, confirmations, cancellations, validation retries, EOF and recoverable exceptions | Plain/TUI regressions and control-specific PTY checks. |

Prompt inventory includes every literal prompt and the dynamic prompt families:
programme/citizenship/material/supplier/subject menus; current editable application
fields; role defaults; named typed workflow data fields; stage/task/role inputs;
instance/student/request references; numeric thresholds and branch roles. These
remain in main; the UI returns the original strings to their existing validators.
No form field, choice, validation condition or workflow operation was removed.

Exact comparison means the complete captured workflow output, with only clock
values normalized, and all generated document bytes. It does not mean every
possible invalid string or every physical terminal configuration was exhaustively
tried. The remaining validation code is unchanged, and tested retry families are
listed above.

## D. Workflow verification

University keeps the original order: register instance/form, collect information,
Intake (`undergrad-submit`, `undergrad-payfee`), Academic Screening
(`undergrad-aps`, `undergrad-english`), Offer Decision (`undergrad-issue-offer`),
result/completion. A failed screening still follows the engine rejection path
and prevents inappropriate later work. The TUI does not skip an engine step.

Procurement keeps Request and Specification (`proc-submit`, `proc-spec`),
Supplier and Cost Review (`proc-quote`, `proc-approve`), result/completion and the
existing displayed deferred Delivery and Inspection (`proc-inspect`). The ledger
still observes actual approved completion; the UI neither approves nor commits.

Generic definitions are created by the retained factory/builder, and execution
continues through the same facade APIs. Automatic processing and manual decisions
are unchanged. Conditional tasks are selected/skipped by the original engine,
not by the renderer. User configuration and record data are untouched.

Document paths remain `documents/` with the same student/request/instance-based
filenames. No builder, factory, template content or file-saving algorithm changed.

## E. Engine and design-pattern preservation

Every pre-existing root `.h`/`.cpp` other than `main.cpp` was frozen before this
work. All protected hashes still match: facade, instances/definitions/context,
components, concrete states, routing, approval handlers, ledger, builders,
Directors, factories/products, observers, caretaker/memento, visitor, adapter and
workflow-template factory are untouched. Backups, archives and original example
documents are also unchanged.

There is no new workflow class, driver, manager, engine or business implementation.
TerminalUI owns presentation resources only. Architecture remains:

```
TerminalUI -> existing main orchestration -> TividyEngineFacade
           -> original workflow/pattern classes -> captured presentation output
```

## F. Build and tests actually executed

Ncurses headers and library stubs were found in the installed Mac SDK. A C++11
compile/link probe with `-lncurses` passed and the real application ran through
that system library. No dependency installation, Git or remote access was used.

Executed:

- `make -j4`: successful C++11 ncurses build, no warnings/errors.
- `make test`: **181 engine checks**, existing shell smoke checks and
  **201 plain-CLI checks passed**.
- `TIVIDY_TEST_TUI=1 TIVIDY_BASELINE="$PWD/tests/tividy_before_tui" python3 tests/run_scenarios.py`:
  **235 checks passed** using actual PTYs/keystrokes and the original inputs.
  Fixed outcomes/output/documents matched the pre-UI executable.
- `python3 tests/test_tui_controls.py`: **69 real-terminal control checks passed**.
  These exercised arrow selection, real mouse dispatch/Submit, form editing,
  F2/history, scroll keys, resizing down and back up, confirmations, cancellation,
  plain mode in a TTY, Ctrl-C/Ctrl-D, SIGTERM and terminal restoration.
- Protected-file hash verification: no engine/pattern, backup, archive or example
  document changed. No `.d` files were created.

`make test-tui` repeats the interactive replay/control suites. Tests use only
Python's standard library and isolated directories inside `tests/`. Trace output
is an explicit test/debug option (`TIVIDY_UI_TRACE`); normal launches do not create
trace files. It captures presentation events, never drives engine operations.

Logs: `tests/tui-build.log`, `tests/tui-after-tests.log`,
`tests/tui-interactive-regressions.log`, `tests/tui-controls.log`.

The first strict terminal-mode assertion found Darwin's transient PENDIN retype
flag. Explicit saved-mode restoration was added; tests verify every persistent
terminal attribute, including ECHO/ICANON/ISIG and control characters, while
excluding that kernel-managed transient flag.

## G. Limits

- Minimum comfortable screen is 50 columns by 18 rows. Smaller screens display
  resize guidance; enlarging restores the current control. Plain mode remains
  available. Tested sizes include 120x32, 80x24, 45x12 and 120x36.
- Mouse operation depends on terminal reporting settings. Xterm-style mouse
  events were exercised through a real PTY, not physical clicks in every Mac
  Terminal/iTerm configuration. Keyboard operation is sufficient for every action.
- The original generic logical-parallel and state-machine limitations remain;
  this presentation task did not implement a different scheduler or state graph.
- Legacy transmission/email remain the original console demonstrations.
- No graphical desktop screenshot or external dependency was required. The
  interactive tests exercised actual ncurses screens and input, not mocked UI calls.

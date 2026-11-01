# Makefile for TIVIDY University Admissions Engine
# COS214 Practical 6
#
# Usage:
#   make          -> build the binary (tividy.exe on Windows, tividy on Unix)
#   make run      -> build and run
#   make clean    -> remove object files and binary
#
# After building, run with:
#   Windows:  .\tividy.exe
#   Linux/mac: ./tividy

CXX      := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -g -I.
LDFLAGS  := -lncurses

# Platform-aware target name
ifeq ($(OS),Windows_NT)
    TARGET := tividy.exe
else
    TARGET := tividy
endif

SRCS := \
    main.cpp \
    TerminalUI.cpp \
    BudgetLedger.cpp \
    WorkflowTemplateFactory.cpp \
    WorkflowContext.cpp \
    WorkflowComponent.cpp \
    CompositeStage.cpp \
    LeafWorkItem.cpp \
    WorkflowDefinition.cpp \
    WorkflowInstance.cpp \
    WorkflowMemento.cpp \
    AuditCaretaker.cpp \
    ActiveState.cpp \
    InactiveState.cpp \
    CompletedState.cpp \
    RejectedState.cpp \
    DefectResolutionState.cpp \
    RoutingStrategy.cpp \
    RoleBasedRouting.cpp \
    ValueThresholdRouting.cpp \
    ApprovalHandler.cpp \
    HandlerA.cpp \
    HandlerB.cpp \
    UniversityRuleHandler.cpp \
    ProcurementRuleHandler.cpp \
    MetricsReportVisitor.cpp \
    NotificationObserver.cpp \
    StateObserver.cpp \
    AuditLoggerObserver.cpp \
    ConcreteWorkflowBuilder.cpp \
    Director.cpp \
    UniversityWorkflowDirector.cpp \
    ProcurementWorkflowDirector.cpp \
    InformationalContract.cpp \
    InformationalGuideline.cpp \
    TransactionalContract.cpp \
    TransactionalGuideline.cpp \
    InformationalFactory.cpp \
    TransactionalFactory.cpp \
    ContractBuilder.cpp \
    GuidelineBuilder.cpp \
    LegacyiHookAPI.cpp \
    LegacyiHookAdapter.cpp \
    TividyEngineFacade.cpp

OBJS := $(SRCS:.cpp=.o)
# Rebuild after header changes without generating dependency files.
HEADERS := $(wildcard *.h *.hpp)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

docs:
	doxygen Doxyfile

run: $(TARGET)
ifeq ($(OS),Windows_NT)
	.\$(TARGET)
else
	./$(TARGET)
endif

clean:
ifeq ($(OS),Windows_NT)
	-del /Q $(OBJS) $(OBJS:.o=.d) $(TARGET) tests/engine_tests tests/engine_tests.d tests/interactive_workflows 2>NUL
else
	rm -f $(OBJS) $(OBJS:.o=.d) $(TARGET) tests/engine_tests tests/engine_tests.d tests/interactive_workflows
endif

.PHONY: all clean run docs

TEST_OBJS := $(filter-out main.o TerminalUI.o,$(OBJS))
tests/engine_tests: tests/engine_tests.cpp $(TEST_OBJS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(TEST_OBJS) $(LDFLAGS)

test: $(TARGET) tests/engine_tests
	./tests/engine_tests
	sh tests/run_golden.sh
	python3 tests/run_scenarios.py

.PHONY: test

# Uses a real PTY and only Python's standard library; no display server is needed.
test-tui: $(TARGET)
	TIVIDY_TEST_TUI=1 python3 tests/run_scenarios.py
	python3 tests/test_tui_controls.py

.PHONY: test-tui

# Focused checks for screened submissions and human review; no extra dependencies.
tests/interactive_workflows: tests/interactive_workflows.cpp $(OBJS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(filter-out main.o,$(OBJS)) $(LDFLAGS)

test-interactive: $(TARGET) tests/interactive_workflows
	./tests/interactive_workflows
	python3 tests/interactive_smoke.py

.PHONY: test-interactive

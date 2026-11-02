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
LDFLAGS  :=

# Platform-aware target name
ifeq ($(OS),Windows_NT)
    TARGET := tividy.exe
else
    TARGET := tividy
endif

SRCS := \
    main.cpp \
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
    MetricsReportVisitor.cpp \
    NotificationObserver.cpp \
    StateObserver.cpp \
    AuditLoggerObserver.cpp \
    ConcreteWorkflowBuilder.cpp \
    Director.cpp \
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

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
ifeq ($(OS),Windows_NT)
	.\$(TARGET)
else
	./$(TARGET)
endif

clean:
ifeq ($(OS),Windows_NT)
	-del /Q $(OBJS) $(TARGET) 2>NUL
else
	rm -f $(OBJS) $(TARGET)
endif

.PHONY: all clean run
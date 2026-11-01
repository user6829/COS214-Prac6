/**
 * @file test.cpp
 * @brief Unit tests for the WorkItemState State design pattern.
 *
 * Build example (single translation unit):
 *   g++ -std=c++17 -I. test.cpp ActiveState.cpp CompletedState.cpp \
 *       DefectResolutionState.cpp InactiveState.cpp RejectedState.cpp \
 *       -o test_runner
 *
 * Run:
 *   ./test_runner
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <memory>
#include <string>
#include <vector>

#include "WorkItemState.h"
#include "ActiveState.h"
#include "CompletedState.h"
#include "DefectResolutionState.h"
#include "InactiveState.h"
#include "RejectedState.h"

// ---------------------------------------------------------------------------
// Helper: capture std::cout output for handle() verification
// ---------------------------------------------------------------------------
#include <sstream>
#include <iostream>

namespace {
    class CoutCapture {
    public:
        CoutCapture() : oldBuf_(std::cout.rdbuf(stream_.rdbuf())) {}
        ~CoutCapture() { std::cout.rdbuf(oldBuf_); }
        std::string str() const { return stream_.str(); }
    private:
        std::ostringstream stream_;
        std::streambuf* oldBuf_;
    };
}

// ---------------------------------------------------------------------------
// getStateName() tests
// ---------------------------------------------------------------------------
TEST_CASE("Each concrete state reports its expected name") {
    ActiveState            active;
    CompletedState         completed;
    DefectResolutionState  defect;
    InactiveState          inactive;
    RejectedState          rejected;

    CHECK(active.getStateName()    == "Active");
    CHECK(completed.getStateName() == "Completed");
    CHECK(defect.getStateName()    == "Defect Resolution");
    CHECK(inactive.getStateName()  == "Inactive");
    CHECK(rejected.getStateName()  == "Rejected");
}

TEST_CASE("getStateName is stable across repeated calls") {
    ActiveState active;
    CHECK(active.getStateName() == active.getStateName());
}

// ---------------------------------------------------------------------------
// Permission tests (canExecute / canApprove)
// ---------------------------------------------------------------------------
TEST_CASE("Default permissions are deny") {
    // WorkItemState base defaults are false; verify each concrete class.
    CompletedState         completed;
    RejectedState          rejected;

    CHECK_FALSE(completed.canExecute());
    CHECK_FALSE(completed.canApprove());
    CHECK_FALSE(rejected.canExecute());
    CHECK_FALSE(rejected.canApprove());
}

TEST_CASE("ActiveState permits approve but not execute") {
    ActiveState active;
    CHECK(active.canApprove());
    CHECK_FALSE(active.canExecute());
}

TEST_CASE("DefectResolutionState permits approve but not execute") {
    DefectResolutionState defect;
    CHECK(defect.canApprove());
    CHECK_FALSE(defect.canExecute());
}

TEST_CASE("InactiveState permits execute but not approve") {
    InactiveState inactive;
    CHECK(inactive.canExecute());
    CHECK_FALSE(inactive.canApprove());
}

// ---------------------------------------------------------------------------
// handle() output tests
// ---------------------------------------------------------------------------
TEST_CASE("ActiveState::handle emits expected message") {
    ActiveState active;
    CoutCapture cap;
    active.handle(nullptr);
    CHECK(cap.str() == "Work item is Active.\n");
}

TEST_CASE("CompletedState::handle emits expected message") {
    CompletedState completed;
    CoutCapture cap;
    completed.handle(nullptr);
    CHECK(cap.str() == "Work item has been completed.\n");
}

TEST_CASE("DefectResolutionState::handle emits expected message") {
    DefectResolutionState defect;
    CoutCapture cap;
    defect.handle(nullptr);
    CHECK(cap.str() == "Work item is undergoing a defect resolution.\n");
}

TEST_CASE("InactiveState::handle emits expected message") {
    InactiveState inactive;
    CoutCapture cap;
    inactive.handle(nullptr);
    CHECK(cap.str() == "Work item is inactive.\n");
}

TEST_CASE("RejectedState::handle emits expected message") {
    RejectedState rejected;
    CoutCapture cap;
    rejected.handle(nullptr);
    CHECK(cap.str() == "Work item has been rejected.\n");
}

// ---------------------------------------------------------------------------
// Polymorphism tests via the WorkItemState interface
// ---------------------------------------------------------------------------
TEST_CASE("All states are usable through the WorkItemState interface") {
    std::vector<std::unique_ptr<WorkItemState>> states;
    states.emplace_back(std::make_unique<ActiveState>());
    states.emplace_back(std::make_unique<CompletedState>());
    states.emplace_back(std::make_unique<DefectResolutionState>());
    states.emplace_back(std::make_unique<InactiveState>());
    states.emplace_back(std::make_unique<RejectedState>());

    const std::vector<std::string> expectedNames = {
        "Active", "Completed", "Defect Resolution", "Inactive", "Rejected"
    };

    REQUIRE(states.size() == expectedNames.size());
    for (size_t i = 0; i < states.size(); ++i) {
        CAPTURE(i);
        CHECK(states[i]->getStateName() == expectedNames[i]);
        // handle() must not crash on nullptr (current implementations ignore it)
        CHECK_NOTHROW(states[i]->handle(nullptr));
    }
}

TEST_CASE("Virtual destructor allows safe polymorphic deletion") {
    WorkItemState* s = new ActiveState();
    CHECK_NOTHROW(delete s);
}
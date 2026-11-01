#include "doctest.h"
#include "AuditLoggerObserver.h"
#include "NotificationObserver.h"
#include "StateObserver.h"
#include "WorkflowInstance.h"
#include "WorkflowContext.h"
#include <memory>
#include <sstream>

static std::unique_ptr<WorkflowInstance> makeInstance() {
    return std::unique_ptr<WorkflowInstance>(new WorkflowInstance(new WorkflowContext()));
}

TEST_CASE("AuditLoggerObserverTest - RecordsRecognizedStateOnce") {
    auto wi = makeInstance();
    wi->getContext()->setData("ID", "WF-1");
    wi->setID();

    AuditLoggerObserver audit;
    audit.setVerbose(false);
    wi->attach(&audit);

    wi->publish("Active", "start");
    wi->publish("Active", "start");

    const auto* h = audit.getHistory("WF-1");
    REQUIRE(h != nullptr);
    REQUIRE(h->size() == 1);
    CHECK((*h)[0].state == "Active");
    CHECK((*h)[0].note  == "start");
}

TEST_CASE("AuditLoggerObserverTest - IgnoresUnrecognizedState") {
    auto wi = makeInstance();
    wi->getContext()->setData("ID", "WF-2");
    wi->setID();
    AuditLoggerObserver audit;
    audit.setVerbose(false);
    wi->attach(&audit);

    wi->publish("BOGUS", "evt");
    const auto* h = audit.getHistory("WF-2");
    CHECK((h == nullptr || h->empty()));
}

TEST_CASE("AuditLoggerObserverTest - RecordsTransitions") {
    auto wi = makeInstance();
    wi->getContext()->setData("ID", "WF-3");
    wi->setID();
    AuditLoggerObserver audit;
    audit.setVerbose(false);
    wi->attach(&audit);

    wi->publish("Active", "start");
    wi->publish("Completed", "finish");

    const auto* h = audit.getHistory("WF-3");
    REQUIRE(h != nullptr);
    REQUIRE(h->size() == 2);
    CHECK((*h)[0].state == "Active");
    CHECK((*h)[1].state == "Completed");
}

TEST_CASE("AuditLoggerObserverTest - GetHistoryUnknownIdReturnsNull") {
    AuditLoggerObserver audit;
    CHECK(audit.getHistory("nope") == nullptr);
}

TEST_CASE("AuditLoggerObserverTest - VerboseOutputContainsEventAndId") {
    auto wi = makeInstance();
    wi->getContext()->setData("ID", "WF-V");
    wi->setID();

    AuditLoggerObserver audit;
    audit.setVerbose(true);
    wi->attach(&audit);

    std::stringstream captured;
    auto* old = std::cout.rdbuf(captured.rdbuf());
    captured.str(""); 
    wi->publish("Active", "go");
    std::cout.rdbuf(old);

    CHECK(captured.str().find("[Audit]") != std::string::npos);
    CHECK(captured.str().find("go")      != std::string::npos);
    CHECK(captured.str().find("WF-V")    != std::string::npos);
}

TEST_CASE("NotificationObserverTest - PrintsWhenVerbose") {
    auto wi = makeInstance();
    wi->getContext()->setData("ID", "N-1");
    wi->setID();
    NotificationObserver n;
    wi->attach(&n);

    std::stringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    cap.str(""); 
    wi->publish("Active", "evt");
    std::cout.rdbuf(old);

    CHECK(cap.str().find("[Notification]") != std::string::npos);
    CHECK(cap.str().find("evt")            != std::string::npos);
    CHECK(cap.str().find("N-1")            != std::string::npos);
}

TEST_CASE("NotificationObserverTest - SilentWhenNotVerbose") {
    auto wi = makeInstance();
    NotificationObserver n;
    n.setVerbose(false);
    wi->attach(&n);

    std::stringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    cap.str(""); 
    wi->publish("Active", "evt");
    std::cout.rdbuf(old);

    CHECK(cap.str().empty());
}

TEST_CASE("StateObserverTest - PrintsStateWhenVerbose") {
    auto wi = makeInstance();
    StateObserver s;
    wi->attach(&s);

    std::stringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    cap.str(""); 
    wi->publish("WAITLISTED", "wait");
    std::cout.rdbuf(old);

    CHECK(cap.str().find("[State]")    != std::string::npos);
    CHECK(cap.str().find("WAITLISTED") != std::string::npos);
}

TEST_CASE("StateObserverTest - SilentWhenNotVerbose") {
    auto wi = makeInstance();
    StateObserver s;
    s.setVerbose(false);
    wi->attach(&s);

    std::stringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    cap.str(""); 
    wi->publish("Active", "go");
    std::cout.rdbuf(old);
    
    CHECK(cap.str().empty());
}
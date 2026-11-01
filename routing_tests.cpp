/**
 * @file testRouting.cpp
 * @brief Doctest unit tests for the TIVIDY Strategy-pattern routing classes.
 *
 * 
 */
#include "doctest.h"

#include "RoutingStrategy.h"
#include "RoleBasedRouting.h"
#include "ValueThresholdRouting.h"
#include "WorkflowContext.h"
#include "LeafWorkItem.h"

#include <limits>
#include <stdexcept>

TEST_SUITE("Routing strategies") {

TEST_CASE("RoutingStrategy: polymorphic dispatch through base interface") {
    WorkflowContext context;
    LeafWorkItem item("route-1", "Review application");
    RoleBasedRouting routing("AdmissionsOfficer");
    const RoutingStrategy& strategy = routing;
    CHECK(strategy.determineAssignee(item, context) == "AdmissionsOfficer");
}

TEST_CASE("RoleBasedRouting: description names the fixed role") {
    RoleBasedRouting routing("AdmissionsOfficer");
    CHECK(routing.describe() == "always AdmissionsOfficer");
}

TEST_CASE("RoleBasedRouting: ignores unrelated workflow data") {
    WorkflowContext context;
    LeafWorkItem item("route-2", "Review application");
    RoleBasedRouting routing("AdmissionsOfficer");
    context.setData("amount", "999999");
    CHECK(routing.determineAssignee(item, context) == "AdmissionsOfficer");
}

TEST_CASE("RoleBasedRouting: rejects an empty role") {
    CHECK_THROWS_AS(RoleBasedRouting(""), std::invalid_argument);
}

TEST_CASE("ValueThresholdRouting: below threshold uses low role") {
    WorkflowContext context;
    LeafWorkItem item("route-3", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "999.99");
    CHECK(routing.determineAssignee(item, context) == "Manager");
}

TEST_CASE("ValueThresholdRouting: equal to threshold uses high role") {
    WorkflowContext context;
    LeafWorkItem item("route-4", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "1000");
    CHECK(routing.determineAssignee(item, context) == "Director");
}

TEST_CASE("ValueThresholdRouting: above threshold uses high role") {
    WorkflowContext context;
    LeafWorkItem item("route-5", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "1000.01");
    CHECK(routing.determineAssignee(item, context) == "Director");
}

TEST_CASE("ValueThresholdRouting: negative numeric value uses low role") {
    WorkflowContext context;
    LeafWorkItem item("route-6", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "-10");
    CHECK(routing.determineAssignee(item, context) == "Manager");
}

TEST_CASE("ValueThresholdRouting: zero numeric value uses low role") {
    WorkflowContext context;
    LeafWorkItem item("route-7", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "0");
    CHECK(routing.determineAssignee(item, context) == "Manager");
}

TEST_CASE("ValueThresholdRouting: description explains threshold") {
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    CHECK(routing.describe() == "amount >= 1000 -> Director, otherwise Manager");
}

TEST_CASE("ValueThresholdRouting: convenience constructor reads amount") {
    WorkflowContext context;
    LeafWorkItem item("route-8", "Approve requisition");
    ValueThresholdRouting routing(1000.0, "Manager", "Director");
    context.setData("amount", "1000");
    CHECK(routing.determineAssignee(item, context) == "Director");
}

TEST_CASE("ValueThresholdRouting: custom data key is respected") {
    WorkflowContext context;
    LeafWorkItem item("route-9", "Review academic eligibility");
    ValueThresholdRouting routing("aps", 30.0, "Reviewer", "FacultyBoard");
    context.setData("aps", "31");
    CHECK(routing.determineAssignee(item, context) == "FacultyBoard");
}

TEST_CASE("ValueThresholdRouting: rejects an empty key") {
    CHECK_THROWS_AS(ValueThresholdRouting("", 100.0, "A", "B"), std::invalid_argument);
}

TEST_CASE("ValueThresholdRouting: rejects an empty low role") {
    CHECK_THROWS_AS(ValueThresholdRouting("amount", 100.0, "", "B"), std::invalid_argument);
}

TEST_CASE("ValueThresholdRouting: rejects an empty high role") {
    CHECK_THROWS_AS(ValueThresholdRouting("amount", 100.0, "A", ""), std::invalid_argument);
}

TEST_CASE("ValueThresholdRouting: rejects infinite threshold") {
    CHECK_THROWS_AS(ValueThresholdRouting("amount", std::numeric_limits<double>::infinity(), "A", "B"), std::invalid_argument);
}

TEST_CASE("ValueThresholdRouting: rejects NaN threshold") {
    CHECK_THROWS_AS(ValueThresholdRouting("amount", std::numeric_limits<double>::quiet_NaN(), "A", "B"), std::invalid_argument);
}

TEST_CASE("ValueThresholdRouting: rejects empty data value") {
    WorkflowContext context;
    LeafWorkItem item("route-10", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "");
    CHECK_THROWS_AS(routing.determineAssignee(item, context), std::runtime_error);
}

TEST_CASE("ValueThresholdRouting: rejects nonnumeric data") {
    WorkflowContext context;
    LeafWorkItem item("route-11", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "abc");
    CHECK_THROWS_AS(routing.determineAssignee(item, context), std::runtime_error);
}

TEST_CASE("ValueThresholdRouting: rejects trailing nonnumeric characters") {
    WorkflowContext context;
    LeafWorkItem item("route-12", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "1000abc");
    CHECK_THROWS_AS(routing.determineAssignee(item, context), std::runtime_error);
}

TEST_CASE("ValueThresholdRouting: rejects NaN workflow data") {
    WorkflowContext context;
    LeafWorkItem item("route-13", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "nan");
    CHECK_THROWS_AS(routing.determineAssignee(item, context), std::runtime_error);
}

TEST_CASE("ValueThresholdRouting: rejects infinite workflow data") {
    WorkflowContext context;
    LeafWorkItem item("route-14", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "inf");
    CHECK_THROWS_AS(routing.determineAssignee(item, context), std::runtime_error);
}

TEST_CASE("ValueThresholdRouting: missing workflow key throws out_of_range") {
    WorkflowContext context;
    LeafWorkItem item("route-15", "Approve requisition");
    ValueThresholdRouting routing("amount", 1000.0, "Manager", "Director");
    CHECK_THROWS_AS(routing.determineAssignee(item, context), std::out_of_range);
}

} // TEST_SUITE

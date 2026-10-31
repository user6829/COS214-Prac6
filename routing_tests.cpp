#include "RoutingStrategy.h"
#include "RoleBasedRouting.h"
#include "ValueThresholdRouting.h"
#include "WorkflowContext.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <limits>
#include <cmath>

// These routing implementations do not dereference the LeafWorkItem argument.
// A null pointer is never dereferenced; it only supplies a reference for tests.
// Instead of inventing a dummy LeafWorkItem, use an actual LeafWorkItem object.
#include "LeafWorkItem.h"

static int passed = 0;
static int failed = 0;
void check(bool condition, const std::string& name) {
    if (condition) { ++passed; std::cout << "PASS " << name << '\n'; }
    else { ++failed; std::cout << "FAIL " << name << '\n'; }
}
template <typename E, typename F>
void throwsAs(F fn, const std::string& name) {
    try { fn(); check(false, name); }
    catch (const E&) { check(true, name); }
    catch (...) { check(false, name); }
}
int main() {
    WorkflowContext context;
    // Storage is provided by a real LeafWorkItem; the strategy never accesses it.
    // Avoid linking the entire application by using a reference to suitably aligned
    // inert storage, safe only because these implementations do not access the item.
    alignas(LeafWorkItem) unsigned char storage[sizeof(LeafWorkItem)];
    const LeafWorkItem& item = *reinterpret_cast<const LeafWorkItem*>(storage);

    RoleBasedRouting role("AdmissionsOfficer");
    const RoutingStrategy& base = role;
    check(base.determineAssignee(item, context) == "AdmissionsOfficer", "RoleBased: fixed role through abstract Strategy");
    check(role.describe() == "always AdmissionsOfficer", "RoleBased: description");
    context.setData("amount", "999999");
    check(role.determineAssignee(item, context) == "AdmissionsOfficer", "RoleBased: ignores unrelated workflow data");
    throwsAs<std::invalid_argument>([](){ RoleBasedRouting invalid(""); }, "RoleBased: empty role rejected");

    ValueThresholdRouting threshold("amount", 1000.0, "Manager", "Director");
    context.setData("amount", "999.99");
    check(threshold.determineAssignee(item, context) == "Manager", "Threshold: below limit");
    context.setData("amount", "1000");
    check(threshold.determineAssignee(item, context) == "Director", "Threshold: exactly at limit");
    context.setData("amount", "1000.01");
    check(threshold.determineAssignee(item, context) == "Director", "Threshold: above limit");
    context.setData("amount", "-10");
    check(threshold.determineAssignee(item, context) == "Manager", "Threshold: negative numeric input");
    context.setData("amount", "0");
    check(threshold.determineAssignee(item, context) == "Manager", "Threshold: zero numeric input");
    check(threshold.describe() == "amount >= 1000 -> Director, otherwise Manager", "Threshold: description");
    ValueThresholdRouting defaultKey(1000, "Manager", "Director");
    context.setData("amount", "1000");
    check(defaultKey.determineAssignee(item, context) == "Director", "Threshold: convenience constructor uses amount");
    ValueThresholdRouting customKey("aps", 30, "Reviewer", "FacultyBoard");
    context.setData("aps", "31");
    check(customKey.determineAssignee(item, context) == "FacultyBoard", "Threshold: custom key");
    throwsAs<std::invalid_argument>([](){ ValueThresholdRouting x("", 100, "A", "B"); }, "Threshold: empty key rejected");
    throwsAs<std::invalid_argument>([](){ ValueThresholdRouting x("amount", 100, "", "B"); }, "Threshold: empty low role rejected");
    throwsAs<std::invalid_argument>([](){ ValueThresholdRouting x("amount", 100, "A", ""); }, "Threshold: empty high role rejected");
    throwsAs<std::invalid_argument>([](){ ValueThresholdRouting x("amount", std::numeric_limits<double>::infinity(), "A", "B"); }, "Threshold: infinite limit rejected");
    throwsAs<std::invalid_argument>([](){ ValueThresholdRouting x("amount", std::numeric_limits<double>::quiet_NaN(), "A", "B"); }, "Threshold: NaN limit rejected");
    context.setData("amount", "");
    throwsAs<std::runtime_error>([&](){ threshold.determineAssignee(item, context); }, "Threshold: empty value rejected");
    context.setData("amount", "abc");
    throwsAs<std::runtime_error>([&](){ threshold.determineAssignee(item, context); }, "Threshold: nonnumeric value rejected");
    context.setData("amount", "1000abc");
    throwsAs<std::runtime_error>([&](){ threshold.determineAssignee(item, context); }, "Threshold: trailing characters rejected");
    context.setData("amount", "nan");
    throwsAs<std::runtime_error>([&](){ threshold.determineAssignee(item, context); }, "Threshold: NaN data rejected");
    context.setData("amount", "inf");
    throwsAs<std::runtime_error>([&](){ threshold.determineAssignee(item, context); }, "Threshold: infinite data rejected");
    WorkflowContext missing;
    throwsAs<std::out_of_range>([&](){ threshold.determineAssignee(item, missing); }, "Threshold: missing data key throws out_of_range");
    std::cout << "RESULT: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}

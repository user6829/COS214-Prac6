// tests/test_context.cpp
#include "doctest.h"
#include "WorkflowContext.h"
#include <stdexcept>

TEST_CASE("WorkflowContextTest - SetThenGetRoundTrips") {
    WorkflowContext ctx;
    ctx.setData("k", "v");
    CHECK(ctx.getData("k") == "v");
}

TEST_CASE("WorkflowContextTest - GetMissingThrowsOutOfRange") {
    WorkflowContext ctx;
    CHECK_THROWS_AS(ctx.getData("missing"), std::out_of_range);
}

TEST_CASE("WorkflowContextTest - SetOverwrites") {
    WorkflowContext ctx;
    ctx.setData("k", "v1");
    ctx.setData("k", "v2");
    CHECK(ctx.getData("k") == "v2");
}

TEST_CASE("WorkflowContextTest - GetAllReturnsAllEntries") {
    WorkflowContext ctx;
    ctx.setData("a", "1");
    ctx.setData("b", "2");
    const auto& all = ctx.getAll();
    CHECK(all.size() == 2);
    CHECK(all.at("a") == "1");
    CHECK(all.at("b") == "2");
}

TEST_CASE("WorkflowContextTest - ReplaceAllDiscardsPreviousEntries") {
    WorkflowContext ctx;
    ctx.setData("old", "x");
    std::map<std::string,std::string> fresh{{"new","y"}};
    ctx.replaceAll(fresh);
    CHECK(ctx.getAll().size() == 1);
    CHECK(ctx.getData("new") == "y");
    CHECK_THROWS_AS(ctx.getData("old"), std::out_of_range);
}
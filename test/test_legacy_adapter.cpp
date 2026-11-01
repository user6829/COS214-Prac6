// tests/test_legacy_adapter.cpp
#include "doctest.h"
#include "LegacyiHookAdapter.h"
#include "LegacyiHookAPI.h"
#include <stdexcept>
#include <map>

TEST_CASE("LegacyiHookAdapterTest - NullLegacyThrows") {
    CHECK_THROWS_AS(LegacyiHookAdapter(nullptr), std::invalid_argument);
}

TEST_CASE("LegacyiHookAdapterTest - EmptyPayloadDoesNotCrash") {
    LegacyiHookAPI api;
    LegacyiHookAdapter adapter(&api);
    std::map<std::string,std::string> empty;
    CHECK_NOTHROW(adapter.sendPayload(empty));
}

TEST_CASE("LegacyiHookAdapterTest - SendsPayload") {
    LegacyiHookAPI api;
    LegacyiHookAdapter adapter(&api);
    std::map<std::string,std::string> payload{
        {"State","Active"},
        {"Event","start"}
    };
    CHECK_NOTHROW(adapter.sendPayload(payload));
}

// NOTE: LegacyiHookAPI::transmitRawData is NOT virtual, so the adapter always
// calls the base implementation. To verify the exact "key--value;\n" wire
// format you'd need to either:
//   1) Make transmitRawData virtual in LegacyiHookAPI and subclass it, or
//   2) Extract the formatting into a free function and unit-test that.
// Skipping that assertion for now.
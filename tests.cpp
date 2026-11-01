// ============================================================================
// tests/test_contract.cpp
//
// Doctest for the Tividy workflow document system.
//
// This file contains ONLY tests. It includes the production headers and links
// against the production .cpp files. Nothing is copied or reimplemented here.
//
// Build (see build.sh or CMakeLists.txt at the bottom of this file):
//   g++ -std=c++11 -I. -Isrc -Iinclude tests/test_contract.cpp src/*.cpp \
//       -o test_contract && ./test_contract
// ============================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <map>
#include <string>
#include <stdexcept>

// --- Production headers (adjust paths to match your tree) -------------------
#include "Document.h"
#include "Contract.h"
#include "Guideline.h"
#include "DocumentFactory.h"
#include "InformationalFactory.h"
#include "TransactionalFactory.h"
#include "InformationalContract.h"
#include "TransactionalContract.h"
#include "InformationalGuideline.h"
#include "TransactionalGuideline.h"
#include "DocumentationBuilder.h"
#include "ContractBuilder.h"
#include "GuidelineBuilder.h"
#include "Director.h"

// ============================================================================
// Helpers
// ============================================================================
namespace {

// Minimal metadata set used by several tests. Kept in one place so adding a
// new required key is a one-line change.
std::map<std::string, std::string> universityMeta(const std::string& result) {
    std::map<std::string, std::string> v;
    v["document:format"] = "university";
    v["firstName"]       = "Ada";
    v["surname"]         = "Lovelace";
    v["studentNumber"]   = "S123";
    v["ID"]              = "instance-1";
    v["attempt"]         = "1";
    v["programme"]       = "Computer Science";
    v["Result"]          = result;
    return v;
}

std::map<std::string, std::string> procurementApprovedMeta() {
    std::map<std::string, std::string> v;
    v["document:format"]  = "procurement";
    v["Result"]           = "APPROVED";
    v["firstName"]        = "Bob";
    v["surname"]          = "Site";
    v["requesterRole"]    = "site-manager";
    v["requestReference"] = "REQ-00042";
    v["ID"]               = "instance-2";
    v["projectName"]      = "Block A";
    v["material"]         = "Cement";
    v["quantity"]         = "100";
    v["supplier"]         = "ACME";
    v["amount"]           = "12345.5";
    return v;
}

std::map<std::string, std::string> resultMeta() {
    std::map<std::string, std::string> v;
    v["document:format"]        = "result";
    v["ID"]                     = "instance-3";
    v["document:workflow"]      = "undergraduate-admissions";
    v["document:industry"]      = "informational";
    v["document:tuple"]         = "t";
    v["State"]                  = "COMPLETED";
    v["Result"]                 = "ACCEPTED";
    v["document:history:count"] = "0";
    return v;
}

} // namespace

// ============================================================================
// Document hierarchy
// ============================================================================
TEST_SUITE("Document hierarchy") {

    TEST_CASE("Document::render returns empty when no content is set") {
        InformationalGuideline g;
        CHECK(g.render() == "");
    }

    TEST_CASE("Document::render returns content after setData") {
        InformationalGuideline g;
        std::map<std::string, std::string> d;
        d["content"] = "HELLO";
        g.setData(d);
        CHECK(g.render() == "HELLO");
    }

    TEST_CASE("Concrete print() does not crash with empty data") {
        // Exercises the boilerplate fallback path; stdout is not asserted.
        InformationalContract  ic;
        TransactionalContract  tc;
        InformationalGuideline ig;
        TransactionalGuideline tg;
        CHECK_NOTHROW(ic.print());
        CHECK_NOTHROW(tc.print());
        CHECK_NOTHROW(ig.print());
        CHECK_NOTHROW(tg.print());
    }
}

// ============================================================================
// Factories
// ============================================================================
TEST_SUITE("Factories") {

    TEST_CASE("InformationalFactory produces correct types") {
        InformationalFactory f;
        Document* c = f.createInstance("contract");
        Document* g = f.createInstance("guideline");
        CHECK(dynamic_cast<InformationalContract*>(c)  != nullptr);
        CHECK(dynamic_cast<InformationalGuideline*>(g) != nullptr);
        delete c;
        delete g;
    }

    TEST_CASE("TransactionalFactory produces correct types") {
        TransactionalFactory f;
        Document* c = f.createInstance("contract");
        Document* g = f.createInstance("guideline");
        CHECK(dynamic_cast<TransactionalContract*>(c)  != nullptr);
        CHECK(dynamic_cast<TransactionalGuideline*>(g) != nullptr);
        CHECK(f.getType() == "transactional");
        delete c;
        delete g;
    }

    TEST_CASE("Factories throw std::invalid_argument on unknown type") {
        InformationalFactory a;
        TransactionalFactory b;
        CHECK_THROWS_AS(a.createInstance("bogus"), std::invalid_argument);
        CHECK_THROWS_AS(b.createInstance("bogus"), std::invalid_argument);
    }
}

// ============================================================================
// ContractBuilder
// ============================================================================
TEST_SUITE("ContractBuilder") {

    TEST_CASE("buildPart() produces an InformationalContract by default") {
        ContractBuilder b(false);
        std::map<std::string, std::string> d;
        d["content"] = "PAYLOAD";
        b.setData(d);
        Document* doc = b.buildPart();
        REQUIRE(doc != nullptr);
        CHECK(dynamic_cast<InformationalContract*>(doc) != nullptr);
        CHECK(doc->render() == "PAYLOAD");
        delete b.getResult();   // ownership transferred here
    }

    TEST_CASE("buildPart() produces a TransactionalContract when flagged") {
        ContractBuilder b(true);
        std::map<std::string, std::string> d;
        d["content"] = "TX";
        b.setData(d);
        Document* doc = b.buildPart();
        REQUIRE(doc != nullptr);
        CHECK(dynamic_cast<TransactionalContract*>(doc) != nullptr);
        delete b.getResult();
    }

    TEST_CASE("university format produces CONTRACT content") {
        ContractBuilder b;
        b.setWorkflowData(universityMeta("ACCEPTED"));

        Director dir(&b);
        Document* doc = dir.constructDocument();
        REQUIRE(doc != nullptr);

        const std::string r = doc->render();
        CHECK(r.find("TIVIDY UNIVERSITY CONTRACT") != std::string::npos);
        CHECK(r.find("Signature (University)")     != std::string::npos);
        CHECK(r.find("Signature (Applicant)")      != std::string::npos);
        CHECK(r.find("Ada Lovelace")               != std::string::npos);
        delete doc;
    }

    TEST_CASE("result format is accepted") {
        ContractBuilder b;
        CHECK_NOTHROW(b.setWorkflowData(resultMeta()));
    }

    TEST_CASE("rejects procurement format (GuidelineBuilder-only)") {
        ContractBuilder b;
        std::map<std::string, std::string> v;
        v["document:format"] = "procurement";
        CHECK_THROWS_AS(b.setWorkflowData(v), std::invalid_argument);
    }

    TEST_CASE("rejects unknown format") {
        ContractBuilder b;
        std::map<std::string, std::string> v;
        v["document:format"] = "mystery";
        CHECK_THROWS_AS(b.setWorkflowData(v), std::invalid_argument);
    }
}

// ============================================================================
// GuidelineBuilder
// ============================================================================
TEST_SUITE("GuidelineBuilder") {

    TEST_CASE("buildPart() produces an InformationalGuideline by default") {
        GuidelineBuilder b(false);
        std::map<std::string, std::string> d;
        d["content"] = "G";
        b.setData(d);
        Document* doc = b.buildPart();
        REQUIRE(doc != nullptr);
        CHECK(dynamic_cast<InformationalGuideline*>(doc) != nullptr);
        delete b.getResult();
    }

    TEST_CASE("university format produces LETTER content (not CONTRACT)") {
        GuidelineBuilder b;
        b.setWorkflowData(universityMeta("ACCEPTED"));

        Director dir(&b);
        Document* doc = dir.constructDocument();
        REQUIRE(doc != nullptr);

        const std::string r = doc->render();
        CHECK(r.find("TIVIDY UNIVERSITY LETTER")   != std::string::npos);
        CHECK(r.find("Signature (University)")      == std::string::npos);
        delete doc;
    }

    TEST_CASE("procurement approval path") {
        GuidelineBuilder b;
        b.setWorkflowData(procurementApprovedMeta());

        Director dir(&b);
        Document* doc = dir.constructDocument();
        REQUIRE(doc != nullptr);

        const std::string r = doc->render();
        CHECK(r.find("TIVIDY PROCUREMENT APPROVAL LETTER") != std::string::npos);
        CHECK(r.find("PO-00042")      != std::string::npos);
        CHECK(r.find("R12345.50")     != std::string::npos);
        delete doc;
    }

    TEST_CASE("result format is accepted") {
        GuidelineBuilder b;
        CHECK_NOTHROW(b.setWorkflowData(resultMeta()));
    }

    TEST_CASE("rejects unknown format") {
        GuidelineBuilder b;
        std::map<std::string, std::string> v;
        v["document:format"] = "mystery";
        CHECK_THROWS_AS(b.setWorkflowData(v), std::invalid_argument);
    }
}

// ============================================================================
// Director ownership / lifetime
// ============================================================================
TEST_SUITE("Director") {

    TEST_CASE("constructDocument() with no builder returns nullptr") {
        Director d;
        CHECK(d.constructDocument() == nullptr);
    }

    TEST_CASE("constructDocument() transfers ownership via getResult()") {
        ContractBuilder b;
        b.setWorkflowData(universityMeta("ACCEPTED"));

        Director dir(&b);
        Document* doc = dir.constructDocument();
        REQUIRE(doc != nullptr);
        delete doc;   // ~ContractBuilder runs after; result was nulled
        CHECK(true);
    }

    TEST_CASE("repeated constructDocument() calls are safe") {
        ContractBuilder b;
        std::map<std::string, std::string> d;
        d["content"] = "A";
        b.setData(d);

        Director dir(&b);
        Document* first  = dir.constructDocument();
        Document* second = dir.constructDocument();
        REQUIRE(first  != nullptr);
        REQUIRE(second != nullptr);
        CHECK(first  != second);
        CHECK(first->render()  == "A");
        CHECK(second->render() == "A");
        delete first;
        delete second;
    }
}

// ============================================================================
// Hazard pinning
//
// These tests deliberately lock down the CURRENT (buggy) behavior so that
// fixing the underlying issue is an explicit, reviewed change: when the fix
// lands, these tests fail and must be flipped to CHECK_NOTHROW (or positive
// assertions). Do NOT delete them silently.
// ============================================================================
TEST_SUITE("Known hazards (pinned)") {

    TEST_CASE("resultContent throws on missing history count") {
        // DocumentationBuilder::resultContent() calls std::stoi("") if
        // "document:history:count" is absent. Reached via ContractBuilder /
        // GuidelineBuilder when format == "result".
        ContractBuilder b;
        std::map<std::string, std::string> v = resultMeta();
        v.erase("document:history:count");
        CHECK_THROWS_AS(b.setWorkflowData(v), std::invalid_argument);
    }

    TEST_CASE("procurementContent throws on missing amount") {
        GuidelineBuilder b;
        std::map<std::string, std::string> v = procurementApprovedMeta();
        v.erase("amount");
        CHECK_THROWS_AS(b.setWorkflowData(v), std::invalid_argument);
    }

    TEST_CASE("procurementContent throws on short requestReference") {
        GuidelineBuilder b;
        std::map<std::string, std::string> v = procurementApprovedMeta();
        v["requestReference"] = "REQ";  // substr(4) -> out_of_range
        CHECK_THROWS_AS(b.setWorkflowData(v), std::out_of_range);
    }
}
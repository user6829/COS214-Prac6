#include "ContractBuilder.h"
#include "DocumentFactory.h"
#include "InformationalFactory.h"
#include "TransactionalFactory.h"
#include <ctime>
#include <sstream>
#include <stdexcept>

ContractBuilder::~ContractBuilder()
{
    delete result;
}

Document* ContractBuilder::buildPart() {
    delete result;
    result = nullptr;
    // Choose a factory strategy; here we default to informational.
    // In a real system this would be injected/configured.
    DocumentFactory* factory = transactional
        ? static_cast<DocumentFactory*>(new TransactionalFactory())
        : static_cast<DocumentFactory*>(new InformationalFactory());
    result = factory->createInstance("contract");
    result->setData(values);
    delete factory;
    return result;
}

void ContractBuilder::setData(const std::map<std::string, std::string>& input)
{
    values = input;
}

Document* ContractBuilder::getResult() {
    Document* built = result;
    result = nullptr;
    return built;
}

// Shared university document assembly belongs to the existing Builder collaboration.
namespace {
std::string documentValue(const std::map<std::string, std::string>& values, const std::string& key)
{
    const auto found = values.find(key);
    return found == values.end() ? "" : found->second;
}
std::string documentDate()
{
    std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    char date[32] = {};
    if (local) std::strftime(date, sizeof(date), "%Y-%m-%d", local);
    return date;
}
}
std::string DocumentationBuilder::universityContent(
    const std::map<std::string, std::string>& values, bool contract)
{
    const auto value = [&values](const std::string& key) { return documentValue(values, key); };
    std::ostringstream content;
    content << "========================================\n"
            << "TIVIDY UNIVERSITY " << (contract ? "CONTRACT" : "LETTER") << "\n"
            << "========================================\n"
            << "Date: " << documentDate() << "\n"
            << "Applicant: " << value("firstName") << " " << value("surname") << "\n"
            << "Student number: " << value("studentNumber") << "\n"
            << "Instance ID: " << value("ID") << "\n"
            << "Attempt: " << value("attempt") << "\n"
            << "Programme: " << value("programme") << "\n"
            << "Result: " << value("Result") << "\n";
    if (contract) {
        content << "Parties: TIVIDY University and the applicant\n"
                << "Terms: fees, academic requirements and student conduct apply.\n"
                << "Signature (University): ____________________\n"
                << "Signature (Applicant):  ____________________\n";
    } else if (value("Result") == "ACCEPTED") {
        content << "Next steps: accept the offer, pay the applicable fees and register.\n";
    } else {
        content << "Reasons for rejection:\n";
        if (!value("reviewerReason").empty()) content << "- Reviewer: " << value("reviewerReason") << "\n";
        for (int i = 1; i <= 4; ++i) {
            const std::string prefix = "rule:undergrad-aps:" + std::to_string(i) + ":";
            if (value(prefix + "passed") == "false") content << "- " << value(prefix + "reason") << "\n";
        }
    }
    content << "========================================\n";
    return content.str();
}

void ContractBuilder::setWorkflowData(const std::map<std::string, std::string>& input)
{
    values = input;
    const std::string format = documentValue(input, "document:format");
    if (format == "university") values["content"] = universityContent(input, true);
    else if (format == "result") values["content"] = resultContent(input);
    else throw std::invalid_argument("Unsupported contract document format");
}

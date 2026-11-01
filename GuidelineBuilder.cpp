#include "GuidelineBuilder.h"
#include "DocumentFactory.h"
#include "InformationalFactory.h"
#include "TransactionalFactory.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

GuidelineBuilder::~GuidelineBuilder()
{
    delete result;
}

Document* GuidelineBuilder::buildPart() {
    delete result;
    result = nullptr;
    DocumentFactory* factory = transactional
        ? static_cast<DocumentFactory*>(new TransactionalFactory())
        : static_cast<DocumentFactory*>(new InformationalFactory());
    result = factory->createInstance("guideline");
    result->setData(values);
    delete factory;
    return result;
}

void GuidelineBuilder::setData(const std::map<std::string, std::string>& input)
{
    values = input;
}

Document* GuidelineBuilder::getResult() {
    Document* built = result;
    result = nullptr;
    return built;
}

namespace {
std::string documentValue(const std::map<std::string, std::string>& values, const std::string& key)
{
    const auto found = values.find(key);
    return found == values.end() ? "" : found->second;
}
std::string procurementContent(const std::map<std::string, std::string>& values)
{
    const auto value = [&values](const std::string& key) { return documentValue(values, key); };
    std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    char date[32] = {};
    if (local) std::strftime(date, sizeof(date), "%Y-%m-%d", local);
    const bool approved = value("Result") == "APPROVED";
    std::ostringstream content;
    content << "========================================\n"
            << "TIVIDY PROCUREMENT " << (approved ? "APPROVAL LETTER" : "REJECTION LETTER")
            << "\n========================================\n"
            << "Date: " << date << "\n"
            << "Requester: " << value("firstName") << " " << value("surname") << "\n"
            << "Role: " << value("requesterRole") << "\n"
            << "Request reference: " << value("requestReference") << "\n"
            << "Instance ID: " << value("ID") << "\n"
            << "Project: " << value("projectName") << "\n"
            << "Material: " << value("material") << "\n"
            << "Quantity: " << value("quantity") << "\n"
            << "Supplier: " << value("supplier") << "\n"
            << "Estimated cost: R" << std::fixed << std::setprecision(2) << std::stod(value("amount")) << "\n"
            << "Result: " << value("Result") << "\n";
    if (approved) {
        content << "Purchase-order reference: PO-" << value("requestReference").substr(4) << "\n"
                << "Next steps: arrange delivery and complete the inspection.\n";
    } else {
        content << "Reasons for rejection:\n";
        if (!value("reviewerReason").empty()) content << "- Reviewer: " << value("reviewerReason") << "\n";
        const std::string step = value("workitem:proc-approve:status") == "REJECTED" ? "proc-approve" : "proc-quote";
        const std::string countText = value("rule:" + step + ":count");
        const int count = countText.empty() ? 0 : std::stoi(countText);
        for (int i = 1; i <= count; ++i) {
            const std::string prefix = "rule:" + step + ":" + std::to_string(i) + ":";
            if (value(prefix + "passed") == "false") content << "- " << value(prefix + "reason") << "\n";
        }
    }
    content << "========================================\n";
    return content.str();
}
}
std::string DocumentationBuilder::resultContent(const std::map<std::string, std::string>& values)
{
    const auto value = [&values](const std::string& key) { return documentValue(values, key); };
    std::ostringstream content;
    content << "TIVIDY WORKFLOW RESULT\n"
            << "Reference: " << value("ID") << "\n"
            << "Workflow: " << value("document:workflow") << "\n"
            << "Industry: " << value("document:industry") << "\n"
            << "Tuple: " << value("document:tuple") << "\n"
            << "Status: " << value("State") << "\n"
            << "Result: " << value("Result") << "\n\n"
            << "Processing history:\n";
    const int count = std::stoi(value("document:history:count"));
    for (int i = 0; i < count; ++i) {
        const std::string prefix = "document:history:" + std::to_string(i) + ":";
        content << value(prefix + "state") << " | " << value(prefix + "event") << " | " << value(prefix + "note") << "\n";
    }
    return content.str();
}
void GuidelineBuilder::setWorkflowData(const std::map<std::string, std::string>& input)
{
    values = input;
    const std::string format = documentValue(input, "document:format");
    if (format == "university") values["content"] = universityContent(input, false);
    else if (format == "procurement") values["content"] = procurementContent(input);
    else if (format == "result") values["content"] = resultContent(input);
    else throw std::invalid_argument("Unsupported guideline document format");
}

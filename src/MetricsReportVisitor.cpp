#include "MetricsReportVisitor.h"
#include "CompositeStage.h"
#include "LeafWorkItem.h"
#include "WorkItemState.h"
#include <iostream>
#include <sstream>

MetricsReportVisitor::MetricsReportVisitor()
    : totalExecutionTime(0.0), activeItemsCount(0), runningInstancesCount(0) {}

void MetricsReportVisitor::reset()
{
    totalExecutionTime = 0.0;
    activeItemsCount = 0;
    runningInstancesCount = 0;
    stagesCount = 0;
    workItemsCount = 0;
}

void MetricsReportVisitor::visitStage(CompositeStage* stage)
{
    // CompositeStage::accept walks children; count this node exactly once.
    if (stage) ++stagesCount;
}

void MetricsReportVisitor::visitWorkItem(LeafWorkItem* item)
{
    if (!item)
    {
        return;
    }
    ++workItemsCount;
}

void MetricsReportVisitor::recordInstance(const std::map<std::string, std::string>& payload)
{
    const auto state = payload.find("State");
    if (state != payload.end() && state->second != "Completed" && state->second != "Rejected")
        ++runningInstancesCount;
    for (std::map<std::string, std::string>::const_iterator it = payload.begin();
         it != payload.end(); ++it)
    {
        if (it->first.compare(0, 9, "workitem:") == 0 &&
            it->first.size() > 7 &&
            it->first.compare(it->first.size() - 7, 7, ":status") == 0 &&
            it->second == "PENDING_APPROVAL")
        {
            ++activeItemsCount;
        }
    }
}

std::string MetricsReportVisitor::getMetrics()
{
    std::ostringstream out;
    out << "Definition stages: " << stagesCount << "\nWork items: " << workItemsCount << "\n"
        << "Running instances: " << runningInstancesCount << "\n"
        << "Active items awaiting approval: " << activeItemsCount << "\n"
        << "Total execution time: not tracked";
    return out.str();
}

void MetricsReportVisitor::displayMetrics() const
{
    std::cout << "Definition stages: " << stagesCount << "\nWork items: " << workItemsCount << "\n"
              << "Running instances: " << runningInstancesCount << "\n"
              << "Active items awaiting approval: " << activeItemsCount << "\n"
              << "Total execution time: not tracked\n";
}
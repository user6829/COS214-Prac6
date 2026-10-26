#include "MetricsReportVisitor.h"
#include "CompositeStage.h"
#include "LeafWorkItem.h"
#include "WorkItemState.h"
#include <iostream>
#include <sstream>

MetricsReportVisitor::MetricsReportVisitor()
    : totalExecutionTime(0.0), activeItemsCount(0) {}

void MetricsReportVisitor::visitStage(CompositeStage* stage)
{
    // Nothing to measure on a stage itself. CompositeStage::accept()
    // already walks the children, so no recursion is needed here.
    (void)stage;
}

void MetricsReportVisitor::visitWorkItem(LeafWorkItem* item)
{
    if (!item)
    {
        return;
    }
    WorkItemState* state = item->getCurrentState();
    if (state && state->getStateName() == "Active")
    {
        ++activeItemsCount;
    }
}

std::string MetricsReportVisitor::getMetrics()
{
    std::ostringstream out;
    out << "Active items: " << activeItemsCount << "\n"
        << "Total execution time: " << totalExecutionTime << "s";
    return out.str();
}

void MetricsReportVisitor::displayMetrics() const
{
    std::cout << "Active items: " << activeItemsCount << "\n"
              << "Total execution time: " << totalExecutionTime << "s\n";
}
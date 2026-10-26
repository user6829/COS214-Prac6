#ifndef METRICREPORTVISITOR_H
#define METRICREPORTVISITOR_H

#include "WorkflowVisitor.h"
#include <string>

class MetricsReportVisitor : public WorkflowVisitor {
private:
    double totalExecutionTime;
    int activeItemsCount;

public:
    MetricsReportVisitor();
    ~MetricsReportVisitor() override = default;

    void visitStage(CompositeStage* stage) override;
    void visitWorkItem(LeafWorkItem* item) override;
    std::string getMetrics();
    void displayMetrics() const;
};

#endif // METRICREPORTVISITOR_H
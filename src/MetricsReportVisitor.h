/**
 * @file MetricsReportVisitor.h
 * @brief Visitor that traverses workflow definitions and gathers metrics.
 */
#ifndef METRICREPORTVISITOR_H
#define METRICREPORTVISITOR_H

#include "WorkflowVisitor.h"
#include <string>
#include <map>

/**
 * @class MetricsReportVisitor
 * @brief Visitor that traverses workflow definitions and gathers metrics.
 */
class MetricsReportVisitor : public WorkflowVisitor {
private:
    double totalExecutionTime;
    int activeItemsCount;
    int runningInstancesCount;
    int stagesCount{0};
    int workItemsCount{0};

public:
    MetricsReportVisitor();
    ~MetricsReportVisitor() override = default;

    void visitStage(CompositeStage* stage) override;
    void visitWorkItem(LeafWorkItem* item) override;
    void reset();
    void recordInstance(const std::map<std::string, std::string>& payload);
    std::string getMetrics();
    void displayMetrics() const;
};

#endif // METRICREPORTVISITOR_H
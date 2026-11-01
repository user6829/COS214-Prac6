/**
 * @file AuditLoggerObserver.h
 * @brief Observer that records workflow events for audit visibility.
 */
#ifndef AUDITLOGGEROBSERVER_H
#define AUDITLOGGEROBSERVER_H

#include "EventObserver.h"
#include <map>
#include <vector>

    /**
    * @brief AuditLogger for an instance
    */


class AuditLoggerObserver : public EventObserver
{
private:
    bool verbose;
    std::map<std::string, std::string> lastState;

public:
    struct Entry { std::string time; std::string state; std::string note; };
private:
    std::map<std::string, std::vector<Entry> > entries;
public:
    /** Session history for the original guided status/history screens. */
    void record(const std::string& instanceId, const std::string& state, const std::string& note);
    const std::vector<Entry>* getHistory(const std::string& instanceId) const;
    AuditLoggerObserver() : verbose(true) {}
    void setVerbose(bool enabled) { verbose = enabled; }
    void update(SubjectWorkFlowInstance& s);
};
#endif
#include "AuditLoggerObserver.h"
#include <ctime>

void AuditLoggerObserver::update(SubjectWorkFlowInstance& s)
{
    const std::string state = s.getState();
    if ((state == "Inactive" || state == "Active" || state == "DefectResolution" ||
         state == "Rejected" || state == "Completed" || state == "WAITLISTED") &&
        state != lastState[s.getInstanceID()]) {
        record(s.getInstanceID(), state, s.getEventName());
        lastState[s.getInstanceID()] = state;
    }
    if (!verbose) return;
    std::cout<<"[Audit]"<<std::endl;
    std::cout<<"The event "<<s.getEventName()<<" with the ID of "<<s.getInstanceID()<<"."<<std::endl;
}
void AuditLoggerObserver::record(const std::string& id, const std::string& state, const std::string& note)
{
    std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    char buffer[16] = {};
    if (local) std::strftime(buffer, sizeof(buffer), "%H:%M:%S", local);
    entries[id].push_back({buffer, state, note});
}
const std::vector<AuditLoggerObserver::Entry>* AuditLoggerObserver::getHistory(const std::string& id) const
{
    const auto found = entries.find(id);
    return found == entries.end() ? nullptr : &found->second;
}

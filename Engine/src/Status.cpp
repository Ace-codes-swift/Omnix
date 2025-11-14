#include "Status.hpp"

namespace Status {

    // Internal storage for statuses
    static std::string loadingStatus;
    static std::string runtimeStatus;
    static std::string errorStatus;
    static std::string warningStatus;
    static std::string infoStatus;
    static std::string debugStatus;
    static std::string traceStatus;
    static std::string fatalStatus;
    static std::string unknownStatus;
    static std::string successStatus;
    static std::string failureStatus;
    static std::string pendingStatus;
    static std::string cancelledStatus;

    // Setters
    void SetLoadingStatus(const std::string& s) { loadingStatus = s; }
    void SetRuntimeStatus(const std::string& s) { runtimeStatus = s; }
    void SetError(const std::string& s) { errorStatus = s; }
    void SetWarning(const std::string& s) { warningStatus = s; }
    void SetInfo(const std::string& s) { infoStatus = s; }
    void SetDebug(const std::string& s) { debugStatus = s; }
    void SetTrace(const std::string& s) { traceStatus = s; }
    void SetFatal(const std::string& s) { fatalStatus = s; }
    void SetUnknown(const std::string& s) { unknownStatus = s; }
    void SetSuccess(const std::string& s) { successStatus = s; }
    void SetFailure(const std::string& s) { failureStatus = s; }
    void SetPending(const std::string& s) { pendingStatus = s; }
    void SetCancelled(const std::string& s) { cancelledStatus = s; }

    // Getters
    std::string GetLoadingStatus() { std::string out = loadingStatus; loadingStatus = ""; return out; }
    std::string GetRuntimeStatus()  { std::string out = runtimeStatus;  runtimeStatus  = ""; return out; }
    std::string GetError()          { std::string out = errorStatus;    errorStatus    = ""; return out; }
    std::string GetWarning()        { std::string out = warningStatus;  warningStatus  = ""; return out; }
    std::string GetInfo()           { std::string out = infoStatus;     infoStatus     = ""; return out; }
    std::string GetDebug()          { std::string out = debugStatus;    debugStatus    = ""; return out; }
    std::string GetTrace()          { std::string out = traceStatus;    traceStatus    = ""; return out; }
    std::string GetFatal()          { std::string out = fatalStatus;    fatalStatus    = ""; return out; }
    std::string GetUnknown()        { std::string out = unknownStatus;  unknownStatus  = ""; return out; }
    std::string GetSuccess()        { std::string out = successStatus;  successStatus  = ""; return out; }
    std::string GetFailure()        { std::string out = failureStatus;  failureStatus  = ""; return out; }
    std::string GetPending()        { std::string out = pendingStatus;  pendingStatus  = ""; return out; }
    std::string GetCancelled()      { std::string out = cancelledStatus; cancelledStatus= ""; return out; }

    
}
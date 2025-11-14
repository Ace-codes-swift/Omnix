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
    std::string GetLoadingStatus() { return loadingStatus; }
    std::string GetRuntimeStatus() { return runtimeStatus; }
    std::string GetError() { return errorStatus; }
    std::string GetWarning() { return warningStatus; }
    std::string GetInfo() { return infoStatus; }
    std::string GetDebug() { return debugStatus; }
    std::string GetTrace() { return traceStatus; }
    std::string GetFatal() { return fatalStatus; }
    std::string GetUnknown() { return unknownStatus; }
    std::string GetSuccess() { return successStatus; }
    std::string GetFailure() { return failureStatus; }
    std::string GetPending() { return pendingStatus; }
    std::string GetCancelled() { return cancelledStatus; }

}

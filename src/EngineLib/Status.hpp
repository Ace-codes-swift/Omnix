#pragma once
#include <string>
#include <vector>

namespace Status {

    // Log entry with type and message
    enum class LogType {
        Loading,      // light blue
        Runtime,      // light green
        Error,        // crimson red
        Warning,      // light yellowish orange
        Info,         // white
        Debug,        // blue
        Trace,        // purple
        Fatal,        // bright red
        Unknown,      // gray
        Success,      // green
        Failure,      // red
        Pending,      // yellow
        Cancelled     // darkish red
    };

    struct LogEntry {
        LogType type;
        std::string message;
    };

    // Setters - Called by library modules to update the engine status
    void SetLoadingStatus(const std::string& s);
    void SetRuntimeStatus(const std::string& s);
    void SetError(const std::string& s);
    void SetWarning(const std::string& s);
    void SetInfo(const std::string& s);
    void SetDebug(const std::string& s);
    void SetTrace(const std::string& s);
    void SetFatal(const std::string& s);
    void SetUnknown(const std::string& s);
    void SetSuccess(const std::string& s);
    void SetFailure(const std::string& s);
    void SetPending(const std::string& s);
    void SetCancelled(const std::string& s);

    // Getters - Called by the main program to read the status
    std::string GetLoadingStatus();
    std::string GetRuntimeStatus();
    std::string GetError();
    std::string GetWarning();
    std::string GetInfo();
    std::string GetDebug();
    std::string GetTrace();
    std::string GetFatal();
    std::string GetUnknown();
    std::string GetSuccess();
    std::string GetFailure();
    std::string GetPending();
    std::string GetCancelled();

    // Get all log entries for display
    const std::vector<LogEntry>& GetAllLogs();
    void ClearAllLogs();
}

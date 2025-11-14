#pragma once
#include <string>

namespace Status {

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
}

#pragma once

#include <string>
#include <filesystem>

class EngineInit {
public:
    void Init(int StartEngineMode = 1, std::string ProjectName = "None", bool NewProject = true); 
};

namespace fs = std::filesystem;
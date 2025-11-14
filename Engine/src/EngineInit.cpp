#include "EngineInit.hpp"
#include <iostream>
#include "entt.hpp"
#include <filesystem>
#include "Status.hpp"



namespace fs = std::filesystem;

std::string home = getenv("HOME");
std::string omnix_projects = "/Users/" + home + "/Library/Application Support/Omnix/Omnix Projects";



void EngineInit::Init(int StartEngineMode, std::string ProjectName, bool NewProject) {
    Status::SetLoadingStatus("Engine initializing in mode " + std::to_string(StartEngineMode));
    if (NewProject) {
       
        Status::SetLoadingStatus("Creating Project Directory");
        fs::create_directory(ProjectName);
        Status::SetLoadingStatus("Setting Project Template");
        fs::copy("Engine/ProjTemplates/" + std::to_string(StartEngineMode), fs::path(omnix_projects + "/" + ProjectName), fs::copy_options::recursive);
        Status::SetLoadingStatus("Project Template set");
    }
    Status::SetLoadingStatus("Engine initialized in mode " + std::to_string(StartEngineMode) + " in project " + ProjectName);
}

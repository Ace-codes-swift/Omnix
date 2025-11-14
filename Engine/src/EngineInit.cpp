#include "EngineInit.hpp"
#include <iostream>
#include "entt.hpp"
#include <filesystem>
#include "Status.hpp"
#include <CoreFoundation/CoreFoundation.h>


namespace fs = std::filesystem;

std::string home = getenv("HOME"); // already includes /Users/USERNAME
std::string omnix_projects = home + "/Library/Application Support/Omnix/Omnix Projects";



std::string GetResourcePath(const std::string& filename) {
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFStringRef cfName = CFStringCreateWithCString(NULL, filename.c_str(), kCFStringEncodingUTF8);
    CFURLRef resourceURL = CFBundleCopyResourceURL(mainBundle, cfName, NULL, NULL);
    CFRelease(cfName);
    if (!resourceURL) return ""; // file not found
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourceURL, TRUE, (UInt8*)path, PATH_MAX)) {
        CFRelease(resourceURL);
        return "";
    }
    CFRelease(resourceURL);
    return std::string(path);
}



void EngineInit::Init(int StartEngineMode, std::string ProjectName, bool NewProject) {
    Status::SetLoadingStatus("Engine initializing in mode " + std::to_string(StartEngineMode));

    if (NewProject) {
        Status::SetLoadingStatus("Creating Project Directory");
        fs::create_directories(omnix_projects + "/" + ProjectName);

        Status::SetLoadingStatus("Setting Project Template");
        std::string templatePath = GetResourcePath("ProjTemplates/" + std::to_string(StartEngineMode));

        if (templatePath.empty()) {
            Status::SetLoadingStatus("Template not found!");
            return; 
        }

        try {
            fs::copy(templatePath, fs::path(omnix_projects + "/" + ProjectName), fs::copy_options::recursive);
        } catch(const std::exception& e) {
            Status::SetLoadingStatus(std::string("Failed to copy template: ") + e.what());
            return;
        }

        Status::SetLoadingStatus("Project Template set");
    }

    Status::SetLoadingStatus("Engine initialized in mode " + std::to_string(StartEngineMode) + " in project " + ProjectName);
}



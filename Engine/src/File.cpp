#include <string>
#include <filesystem>
#include <fstream>
#include "Status.hpp"
#include "File.hpp"

namespace fs = std::filesystem;

static int fileCount = 1;


void File::CreateFile(std::string filePath, std::string fileName) {
    fs::path file = fs::path(filePath) / fileName;

    // Ensure parent directory exists
    std::error_code ec;
    fs::create_directories(file.parent_path(), ec);

    if (fs::exists(file)) {
        Status::SetRuntimeStatus("File already exists");
        return;
    }

    std::ofstream ofs(file);
    if (!ofs.is_open()) {
        Status::SetRuntimeStatus("Failed to create file");
        return;
    }
    ofs.close();

    Status::SetRuntimeStatus("File created");
}

void File::DeleteFile(std::string filePath, std::string fileName) {
    fs::path file = fs::path(filePath) / fileName;
    if(fs::exists(file)) {
        fs::remove(file);
        Status::SetRuntimeStatus("File deleted");
    }
}           

void File::RenameFile(std::string filePath,std::string fileName, std::string newFileName) {
    fs::path file = fs::path(filePath) / fileName;
    if(fs::exists(file)) {
        fs::rename(file, fs::path(filePath) / newFileName);
        Status::SetRuntimeStatus("File renamed");
    }
}

void File::CopyFile(std::string sourceFilePath, std::string destinationFilePath, std::string sourceFileName, std::string destinationFileName) {
    fs::path sourceFile = fs::path(sourceFilePath) / sourceFileName;
    fs::path destinationFile = fs::path(destinationFilePath) / destinationFileName;


    if(fs::exists(sourceFile)) {
        fs::copy(sourceFile, destinationFile);
        Status::SetRuntimeStatus("File copied");
    }
}

void File::DuplicateFile(std::string filePath, std::string fileName) {
    fs::path file = fs::path(filePath) / fileName;
    if(fs::exists(file)) {
        // Handle extension properly: test.txt -> test(1).txt
        fs::path p(fileName);
        std::string stem = p.stem().string();
        std::string ext = p.extension().string();
        std::string duplicatedFileName = stem + "(" + std::to_string(fileCount) + ")" + ext;
        fs::copy(file, fs::path(filePath) / duplicatedFileName);
        fileCount++;
        Status::SetRuntimeStatus("File duplicated");
    }
}   

void File::CreateDirectory(std::string filePath, std::string directoryName) {
    fs::path directory = fs::path(filePath) / directoryName;
    if(fs::exists(directory)) {
        Status::SetRuntimeStatus("Directory already exists");
        return;
    }
    fs::create_directory(directory);
    Status::SetRuntimeStatus("Directory created");
}   

void File::DeleteDirectory(std::string filePath, std::string directoryName) {
    fs::path directory = fs::path(filePath) / directoryName;
    if(fs::exists(directory)) {
        fs::remove_all(directory);
        Status::SetRuntimeStatus("Directory deleted");
    }
}
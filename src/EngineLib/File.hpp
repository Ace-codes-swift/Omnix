#pragma once

#include <string>
#include <filesystem>


class File {
public:
    void CreateFile(std::string filePath, std::string fileName);
    void DeleteFile(std::string filePath, std::string fileName);
    void RenameFile(std::string filePath,std::string fileName, std::string newFileName);
    void CopyFile(std::string sourceFilePath, std::string destinationFilePath, std::string sourceFileName, std::string destinationFileName);
    void DuplicateFile(std::string filePath, std::string fileName);
    void CreateDirectory(std::string filePath, std::string directoryName);
    void DeleteDirectory(std::string filePath, std::string directoryName);
};


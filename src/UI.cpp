#include "UI.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Panels.h"
#include "EngineLib/File.hpp"
#include <OpenGL/gl3.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace {
    // Utility: render text centered within a rectangle
    static inline void RenderCenteredTextInRect(const ImVec2& rectMin, const ImVec2& rectMax, const char* text)
    {
        ImGui::RenderTextClipped(rectMin, rectMax, text ? text : "", nullptr, nullptr, ImVec2(0.5f, 0.5f), nullptr);
    }
    namespace fs = std::filesystem;
    
    // Global project name storage
    static std::string g_projectName = "NewProject"; // Default matches main.cpp

    struct ScriptState {
        std::string scriptsDir;
        std::string currentScriptPath;
        std::string currentScriptName; // with .cs
        fs::file_time_type lastWriteTime{};
        bool awaitingSave = false;
        bool loaded = false;
        char nameBuf[128] = {0};
    };

    struct FileExplorerState {
        std::string rootDir;
        std::string currentDir;
    };

    ScriptState& scriptState() {
        static ScriptState s;
        return s;
    }

    FileExplorerState& fileExplorerState() {
        static FileExplorerState s;
        return s;
    }

    // Resolve a resource inside the app bundle's Resources/ directory.
    std::string GetResourcePath(const std::string& filename) {
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        if (!mainBundle)
            return {};

        CFStringRef cfName = CFStringCreateWithCString(nullptr, filename.c_str(), kCFStringEncodingUTF8);
        if (!cfName)
            return {};

        CFURLRef resourceURL = CFBundleCopyResourceURL(mainBundle, cfName, nullptr, nullptr);
        CFRelease(cfName);
        if (!resourceURL)
            return {};

        char path[PATH_MAX];
        std::string result;
        if (CFURLGetFileSystemRepresentation(resourceURL, true, reinterpret_cast<UInt8*>(path), PATH_MAX)) {
            result = path;
        }
        CFRelease(resourceURL);
        return result;
    }

    std::string getHomeDir() {
        const char* home = std::getenv("HOME");
        return home ? std::string(home) : std::string("/");
    }

    // NOTE: Must stay in sync with EngineInit.cpp project path logic.
    std::string getProjectsRootDir() {
        return getHomeDir() + "/Library/Application Support/Omnix/Omnix Projects";
    }

    // Returns the current project name (synchronized with main.cpp via UI::setProjectName)
    std::string getCurrentProjectName() {
        return g_projectName;
    }

    std::string ensureProjectRootDir() {
        FileExplorerState& fe = fileExplorerState();
        if (!fe.rootDir.empty()) return fe.rootDir;

        std::string root = getProjectsRootDir() + "/" + getCurrentProjectName();
        std::error_code ec;
        fs::create_directories(root, ec);
        fe.rootDir = root;
        return fe.rootDir;
    }

    std::string ensureScriptsDir() {
        ScriptState& st = scriptState();
        if (!st.scriptsDir.empty()) return st.scriptsDir;
        // Scripts live inside the current project folder under "Scripts"
        std::string dir = ensureProjectRootDir() + "/Scripts";
        std::error_code ec;
        fs::create_directories(dir, ec);
        st.scriptsDir = dir;
        return st.scriptsDir;
    }

    // Load a PNG texture using CoreGraphics into an OpenGL texture.
    GLuint LoadTextureFromPNG(const std::string& path) {
        CGDataProviderRef provider = CGDataProviderCreateWithFilename(path.c_str());
        if (!provider)
            return 0;

        CGImageRef image = CGImageCreateWithPNGDataProvider(provider, nullptr, true, kCGRenderingIntentDefault);
        CGDataProviderRelease(provider);
        if (!image)
            return 0;

        const size_t width  = CGImageGetWidth(image);
        const size_t height = CGImageGetHeight(image);
        if (width == 0 || height == 0) {
            CGImageRelease(image);
            return 0;
        }

        std::vector<unsigned char> pixels(width * height * 4);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(
            pixels.data(),
            static_cast<size_t>(width),
            static_cast<size_t>(height),
            8,
            static_cast<size_t>(width) * 4,
            colorSpace,
            kCGImageAlphaPremultipliedLast
        );
        CGColorSpaceRelease(colorSpace);

        if (!context) {
            CGImageRelease(image);
            return 0;
        }

        CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
        CGContextRelease(context);
        CGImageRelease(image);

        GLuint texId = 0;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        return texId;
    }

    ImTextureRef getFolderIconTexture() {
        static ImTextureRef texRef;      // default-constructed = invalid
        static bool initialized = false;
        if (initialized)
            return texRef;
        initialized = true;

        // Path inside app bundle Resources
        std::string path = GetResourcePath("Assets/macos-folder-blue512x512@2x.png");
        if (path.empty())
            return texRef;

        GLuint texId = LoadTextureFromPNG(path);
        if (texId != 0) {
            texRef = ImTextureRef(reinterpret_cast<void*>(static_cast<intptr_t>(texId)));
        }
        return texRef;
    }

    static bool isAlphaNumOrUnderscore(char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
    }

    std::string sanitizeClassName(const std::string& raw) {
        std::string name;
        name.reserve(raw.size());
        for (char c : raw) {
            name.push_back(isAlphaNumOrUnderscore(c) ? c : '_');
        }
        if (name.empty()) name = "NewScript";
        if (name[0] >= '0' && name[0] <= '9') name = std::string("_") + name;
        return name;
    }

    std::string ensureCSExt(const std::string& raw) {
        if (raw.size() >= 3 && raw.rfind(".cs") == raw.size() - 3) return raw;
        return raw + ".cs";
    }

    bool writeCSharpTemplate(const std::string& path, const std::string& className) {
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out.is_open()) return false;
        out << "using System;\n\n";
        out << "public class " << className << "\n";
        out << "{\n";
        out << "    public void Start()\n";
        out << "    {\n";
        out << "        // Initialization code\n";
        out << "    }\n\n";
        out << "    public void Update()\n";
        out << "    {\n";
        out << "        // Per-frame code\n";
        out << "    }\n";
        out << "}\n";
        out.close();
        return true;
    }

    bool tryOpenWithVSCode(const std::string& path) {
        // Prefer app bundle invocation
        std::string cmd = std::string("open -a \"Visual Studio Code\" \"") + path + "\"";
        int rc = std::system(cmd.c_str());
        if (rc == 0) return true;
        // Fallback to 'code' CLI if present
        rc = std::system((std::string("command -v code >/dev/null 2>&1 && code \"") + path + "\"").c_str());
        if (rc == 0) return true;
        // Fallback to default handler
        rc = std::system((std::string("open \"") + path + "\"").c_str());
        return rc == 0;
    }
}

void UI::draw() {
    
    if (Panels::showOmnix) {
        ImGui::SetNextWindowBgAlpha(1.0f); 
        if (ImGui::Begin("Omnix", &Panels::showOmnix)) {
            ImGui::Text("Hello, Omnix!");
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        }
        ImGui::End();
    }
    
    if (Panels::showAssets) {
        ImGui::SetNextWindowBgAlpha(1.0f); 
        if (ImGui::Begin("Assets", &Panels::showAssets)) {
            // Mini file explorer rooted at the current project folder
            FileExplorerState& fe = fileExplorerState();
            std::string rootDir = ensureProjectRootDir();
            if (fe.currentDir.empty()) {
                fe.currentDir = rootDir;
            }
            std::string dir = fe.currentDir;

            // Navigation bar: Up button + current path (relative to root where possible)
            {
                if (ImGui::Button("Back")) {
                    if (dir != rootDir) {
                        fs::path p(dir);
                        fs::path parent = p.parent_path();
                        std::string parentStr = parent.string();
                        if (parentStr.size() >= rootDir.size() &&
                            parentStr.compare(0, rootDir.size(), rootDir) == 0) {
                            fe.currentDir = parentStr;
                        } else {
                            fe.currentDir = rootDir;
                        }
                        dir = fe.currentDir;
                    }
                }
                ImGui::SameLine();

                std::string displayPath;
                if (dir.size() >= rootDir.size() &&
                    dir.compare(0, rootDir.size(), rootDir) == 0) {
                    displayPath = dir.substr(rootDir.size());
                    if (displayPath.empty()) displayPath = "/";
                } else {
                    displayPath = dir;
                }
                ImGui::TextUnformatted(displayPath.c_str());
            }

            struct FileItem {
                std::string name;
                std::string path;
                bool isDir;
                bool isCS;
            };

            std::vector<FileItem> items;
            {
                std::error_code ec;
                if (fs::exists(dir, ec) && fs::is_directory(dir, ec)) {
                    for (auto it = fs::directory_iterator(dir, ec); !ec && it != fs::end(it); it.increment(ec)) {
                        if (ec) break;
                        const fs::directory_entry& de = *it;
                        const fs::path& p = de.path();
                        FileItem item;
                        item.name = p.filename().string();

                        // Skip Finder metadata and hidden dot-files like ".DS_Store"
                        if (!item.name.empty() && item.name[0] == '.')
                            continue;

                        item.path = p.string();
                        item.isDir = de.is_directory(ec);
                        item.isCS = (!item.isDir && p.extension() == ".cs");
                        items.push_back(std::move(item));
                    }
                }
            }

            // Sort: directories first, then files, both alphabetically
            std::sort(items.begin(), items.end(), [](const FileItem& a, const FileItem& b) {
                if (a.isDir != b.isDir) return a.isDir > b.isDir;
                return a.name < b.name;
            });

            // Selection state - tracks currently selected item
            static std::string selectedPath;
            static bool selectedIsDir = false;
            static bool hasSelection = false;
            
            // Context state for right-click menu over the Assets panel
            static std::string ctxPath;
            static bool ctxIsDir = false;
            static bool ctxHasSelection = false;
            // Clipboard state for Copy/Paste (files only)
            static std::string clipboardPath;
            static bool clipboardHasFile = false;
            // Text buffers for new file/folder naming and rename popup
            static char newFileNameBuf[256] = {0};
            static char newFolderNameBuf[256] = {0};
            static char renameBuf[256] = {0};
            // Base dirs for new file/folder popups
            static std::string newFileBaseDir;
            static std::string newFolderBaseDir;
            // Rename state
            static bool shouldOpenRenamePopup = false;
            static std::string renameTargetPath;
            static bool renameTargetIsDir = false;
            // New File/Folder popup flags
            static bool shouldOpenNewFilePopup = false;
            static bool shouldOpenNewFolderPopup = false;

            // Grid settings
            const float iconSize = 72.0f; // 2x previous size
            const float labelHeight = ImGui::GetTextLineHeight();
            const float cellWidth = 120.0f;
            const float cellHeight = iconSize + 6.0f + labelHeight + 8.0f;
            const float availX = ImGui::GetContentRegionAvail().x;
            int cols = (int)std::max(1.0f, floorf(availX / cellWidth));

            int col = 0;
            for (auto& item : items) {
                const std::string& name = item.name;
                const std::string& path = item.path;
                const bool isDir = item.isDir;

                if (col > 0) ImGui::SameLine();
                ImGui::BeginGroup();

                // Icon button: use PNG icon for folders when available, otherwise fallback to colored square
                ImGui::PushID(path.c_str());
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                // Center icon horizontally within the cell
                float iconX = p0.x + (cellWidth - iconSize) * 0.5f;
                ImVec2 ip0 = ImVec2(iconX, p0.y);
                ImVec2 ip1 = ImVec2(iconX + iconSize, p0.y + iconSize);
                ImDrawList* dl = ImGui::GetWindowDrawList();

                // Check if this item is selected
                bool isSelected = (hasSelection && selectedPath == path);

                // Draw selection highlight if selected
                if (isSelected) {
                    ImVec2 highlightMin = ImVec2(p0.x - 2.0f, p0.y - 2.0f);
                    ImVec2 highlightMax = ImVec2(p0.x + cellWidth + 2.0f, p0.y + cellHeight + 2.0f);
                    ImU32 selectionColor = IM_COL32(70, 130, 180, 80); // Semi-transparent blue
                    ImU32 selectionBorder = IM_COL32(70, 130, 180, 200);
                    dl->AddRectFilled(highlightMin, highlightMax, selectionColor, 4.0f);
                    dl->AddRect(highlightMin, highlightMax, selectionBorder, 4.0f, 0, 2.0f);
                }

                ImTextureRef folderTex = getFolderIconTexture();
                const bool hasFolderIcon = (folderTex != ImTextureRef());

                if (item.isDir && hasFolderIcon) {
                    // Draw the folder PNG
                    dl->AddImage(folderTex, ip0, ip1);
                } else {
                    // Fallback: colored square (used for non-folder items or if texture failed to load)
                    ImU32 iconBg;
                    if (item.isDir) {
                        iconBg = IM_COL32(200, 180, 60, 255); // folder-ish color
                    } else if (item.isCS) {
                        iconBg = IM_COL32(60, 120, 200, 255); // scripts
                    } else {
                        iconBg = IM_COL32(100, 100, 100, 255); // generic file
                    }
                    const ImU32 iconBorder = IM_COL32(30, 60, 100, 255);
                    dl->AddRectFilled(ip0, ip1, iconBg, 6.0f);
                    dl->AddRect(ip0, ip1, iconBorder, 6.0f, 0, 2.0f);
                }
                ImGui::SetCursorScreenPos(ip0);
                ImGui::InvisibleButton("icon", ImVec2(iconSize, iconSize));

                bool iconHovered       = ImGui::IsItemHovered();
                bool iconClicked       = ImGui::IsItemClicked(ImGuiMouseButton_Left);
                bool iconDoubleClicked = iconHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

                // Single click to select
                if (iconClicked && !iconDoubleClicked) {
                    hasSelection = true;
                    selectedPath = path;
                    selectedIsDir = isDir;
                }

                // Drag source from the icon - allow dragging all files and folders
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    // Use different payload types for scripts vs general files
                    if (item.isCS) {
                        ImGui::SetDragDropPayload("OMNIX_SCRIPT_PATH", path.c_str(), path.size() + 1);
                    } else {
                        ImGui::SetDragDropPayload("OMNIX_FILE_PATH", path.c_str(), path.size() + 1);
                    }
                    ImGui::TextUnformatted(name.c_str());
                    ImGui::EndDragDropSource();
                }

                // Drop target on folders - allow dropping files into folders
                if (item.isDir && ImGui::BeginDragDropTarget()) {
                    // Accept both script and general file drops
                    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("OMNIX_FILE_PATH");
                    if (!payload) {
                        payload = ImGui::AcceptDragDropPayload("OMNIX_SCRIPT_PATH");
                    }
                    
                    if (payload) {
                        const char* droppedPath = reinterpret_cast<const char*>(payload->Data);
                        if (droppedPath && std::string(droppedPath) != path) {
                            // Move the file/folder to the target directory
                            std::error_code ec;
                            fs::path srcPath(droppedPath);
                            fs::path dstPath = fs::path(path) / srcPath.filename();
                            
                            // Avoid overwriting existing files
                            if (fs::exists(dstPath, ec)) {
                                std::string baseName = srcPath.stem().string();
                                std::string ext = srcPath.extension().string();
                                int copyIndex = 1;
                                do {
                                    std::string newName = baseName + " " + std::to_string(copyIndex) + ext;
                                    dstPath = fs::path(path) / newName;
                                    ++copyIndex;
                                } while (fs::exists(dstPath, ec));
                            }
                            
                            // Perform the move
                            fs::rename(srcPath, dstPath, ec);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Right-click directly on the icon -> set both persistent selection and context selection
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    hasSelection = true;
                    selectedPath = path;
                    selectedIsDir = isDir;
                    ctxHasSelection = true;
                    ctxPath = path;
                    ctxIsDir = isDir;
                    ImGui::OpenPopup("AssetsContextMenu");
                }

                // Name label under the icon (centered relative to icon, clipped to cell)
                {
                    const float labelH = labelHeight + 8.0f;
                    float labelY = p0.y + iconSize + 6.0f;
                    ImVec2 clipMin = ImVec2(p0.x, labelY);
                    ImVec2 clipMax = ImVec2(p0.x + cellWidth, labelY + labelH);
                    ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
                    float centerX = ip0.x + iconSize * 0.5f;
                    ImVec2 textPos = ImVec2(centerX - textSize.x * 0.5f,
                                             labelY + (labelH - textSize.y) * 0.5f);
                    dl->PushClipRect(clipMin, clipMax, true);
                    dl->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), name.c_str());
                    dl->PopClipRect();
                }

                // Row selectable for double click as well
                if (iconDoubleClicked) {
                    if (item.isDir) {
                        fe.currentDir = path;
                    } else {
                        tryOpenWithVSCode(path);
                    }
                }

                // Also allow interactions by the label area
                ImGui::SetNextItemAllowOverlap();
                ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + iconSize + 6.0f));
                ImGui::InvisibleButton("lbl", ImVec2(cellWidth, labelHeight + 8.0f));
                
                bool labelClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
                bool labelDoubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                
                // Single click on label to select
                if (labelClicked && !labelDoubleClicked) {
                    hasSelection = true;
                    selectedPath = path;
                    selectedIsDir = isDir;
                }
                
                // Double-click on label to open/navigate
                if (labelDoubleClicked) {
                    if (isDir) {
                        fe.currentDir = path;
                    } else {
                        tryOpenWithVSCode(path);
                    }
                }
                // Begin drag from label as well - allow dragging all files and folders
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    if (item.isCS) {
                        ImGui::SetDragDropPayload("OMNIX_SCRIPT_PATH", path.c_str(), path.size() + 1);
                    } else {
                        ImGui::SetDragDropPayload("OMNIX_FILE_PATH", path.c_str(), path.size() + 1);
                    }
                    ImGui::TextUnformatted(name.c_str());
                    ImGui::EndDragDropSource();
                }

                // Drop target on folders from label area as well
                if (item.isDir && ImGui::BeginDragDropTarget()) {
                    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("OMNIX_FILE_PATH");
                    if (!payload) {
                        payload = ImGui::AcceptDragDropPayload("OMNIX_SCRIPT_PATH");
                    }
                    
                    if (payload) {
                        const char* droppedPath = reinterpret_cast<const char*>(payload->Data);
                        if (droppedPath && std::string(droppedPath) != path) {
                            std::error_code ec;
                            fs::path srcPath(droppedPath);
                            fs::path dstPath = fs::path(path) / srcPath.filename();
                            
                            // Avoid overwriting existing files
                            if (fs::exists(dstPath, ec)) {
                                std::string baseName = srcPath.stem().string();
                                std::string ext = srcPath.extension().string();
                                int copyIndex = 1;
                                do {
                                    std::string newName = baseName + " " + std::to_string(copyIndex) + ext;
                                    dstPath = fs::path(path) / newName;
                                    ++copyIndex;
                                } while (fs::exists(dstPath, ec));
                            }
                            
                            fs::rename(srcPath, dstPath, ec);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Right-click on this row (label area) -> set both persistent selection and context selection
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    hasSelection = true;
                    selectedPath = path;
                    selectedIsDir = isDir;
                    ctxHasSelection = true;
                    ctxPath = path;
                    ctxIsDir = isDir;
                    ImGui::OpenPopup("AssetsContextMenu");
                }

                ImGui::PopID();
                ImGui::EndGroup();

                col = (col + 1) % cols;
            }

            // Left-click on empty space: deselect
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !ImGui::IsAnyItemHovered())
            {
                hasSelection = false;
                selectedPath.clear();
                selectedIsDir = false;
            }

            // Right-click on empty space in the Assets window: open context menu with no item selection
            // but use persistent selection if available
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
                !ImGui::IsAnyItemHovered())
            {
                ctxHasSelection = hasSelection;
                ctxPath = selectedPath;
                ctxIsDir = selectedIsDir;
                ImGui::OpenPopup("AssetsContextMenu");
            }

            // Shared context menu for the Assets panel
            if (ImGui::BeginPopup("AssetsContextMenu")) {
                File fileOps;

                bool hasSelection    = ctxHasSelection;
                bool selectedIsDir   = ctxIsDir;
                bool selectedIsFile  = ctxHasSelection && !ctxIsDir;
                bool canOpenInVSCode = selectedIsFile;

                // ---- Create section ----
                if (ImGui::MenuItem("New Folder")) {
                    // Use right-clicked folder as base if any, otherwise current dir
                    newFolderBaseDir = (ctxHasSelection && ctxIsDir) ? ctxPath : dir;
                    newFolderNameBuf[0] = '\0';
                    shouldOpenNewFolderPopup = true;
                }

                if (ImGui::MenuItem("New File...")) {
                    // Use right-clicked folder as base if any, otherwise current dir
                    newFileBaseDir = (ctxHasSelection && ctxIsDir) ? ctxPath : dir;
                    newFileNameBuf[0] = '\0';
                    shouldOpenNewFilePopup = true;
                }

                // ---- Selection-specific section ----
                if (ImGui::MenuItem("Open in VS Code", nullptr, false, canOpenInVSCode)) {
                    tryOpenWithVSCode(ctxPath);
                }

                if (ImGui::MenuItem("Duplicate", nullptr, false, hasSelection)) {
                    fs::path p(ctxPath);
                    std::string parentDir = p.parent_path().string();
                    std::string baseName  = p.filename().string();
                    fileOps.DuplicateFile(parentDir, baseName);
                }

                // Rename (file or folder)
                if (ImGui::MenuItem("Rename...", nullptr, false, ctxHasSelection)) {
                    fs::path p(ctxPath);
                    std::string baseName = p.filename().string();
                    std::snprintf(renameBuf, sizeof(renameBuf), "%s", baseName.c_str());
                    renameTargetPath = ctxPath;
                    renameTargetIsDir = ctxIsDir;
                    shouldOpenRenamePopup = true;
                }

                // Copy / Paste (files only for CopyFile API)
                if (ImGui::MenuItem("Copy", nullptr, false, selectedIsFile)) {
                    clipboardPath    = ctxPath;
                    clipboardHasFile = true;
                }

                bool canPaste = clipboardHasFile;
                if (ImGui::MenuItem("Paste", nullptr, false, canPaste)) {
                    if (clipboardHasFile) {
                        fs::path src(clipboardPath);
                        std::string srcDir  = src.parent_path().string();
                        std::string srcName = src.filename().string();

                        // Destination directory is the current Assets dir
                        fs::path dstDir(dir);
                        fs::path dstPath = dstDir / srcName;

                        // Avoid overwriting existing files by appending " copy" suffix if needed
                        std::string dstName = srcName;
                        int copyIndex = 1;
                        while (fs::exists(dstPath)) {
                            // insert " copy" before extension if present
                            std::string stem  = src.stem().string();
                            std::string ext   = src.extension().string();
                            dstName = stem + " copy";
                            if (copyIndex > 1) {
                                dstName += " " + std::to_string(copyIndex);
                            }
                            dstName += ext;
                            dstPath = dstDir / dstName;
                            ++copyIndex;
                        }

                        fileOps.CopyFile(srcDir, dir, srcName, dstName);
                    }
                }

                if (ImGui::MenuItem("Delete", nullptr, false, hasSelection)) {
                    fs::path p(ctxPath);
                    std::string parentDir = p.parent_path().string();
                    std::string baseName  = p.filename().string();
                    if (selectedIsDir) {
                        fileOps.DeleteDirectory(parentDir, baseName);
                    } else {
                        fileOps.DeleteFile(parentDir, baseName);
                    }
                }

                ImGui::EndPopup();
            }

            // Open popups if flagged (must be outside other popups)
            if (shouldOpenRenamePopup) {
                shouldOpenRenamePopup = false;
                ImGui::OpenPopup("AssetsRenamePopup");
            }
            if (shouldOpenNewFilePopup) {
                shouldOpenNewFilePopup = false;
                ImGui::OpenPopup("AssetsNewFilePopup");
            }
            if (shouldOpenNewFolderPopup) {
                shouldOpenNewFolderPopup = false;
                ImGui::OpenPopup("AssetsNewFolderPopup");
            }

            // New File popup (pre-creation naming, similar to Create C# Script)
            if (ImGui::BeginPopupModal("AssetsNewFilePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextUnformatted("New file name:");
                bool enterPressed = ImGui::InputText("##assets_new_file_name", newFileNameBuf, sizeof(newFileNameBuf), ImGuiInputTextFlags_EnterReturnsTrue);

                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere(-1);
                }

                bool createRequested = false;
                if (ImGui::Button("Create") || enterPressed) createRequested = true;
                ImGui::SameLine();
                if (ImGui::Button("Cancel##assets_new_file_cancel")) {
                    ImGui::CloseCurrentPopup();
                }

                if (createRequested) {
                    std::string rawName = newFileNameBuf;
                    // trim spaces
                    while (!rawName.empty() && (rawName.back() == ' ' || rawName.back() == '\t'))
                        rawName.pop_back();
                    size_t p = 0;
                    while (p < rawName.size() && (rawName[p] == ' ' || rawName[p] == '\t'))
                        ++p;
                    rawName.erase(0, p);
                    if (rawName.empty()) rawName = "New File";

                    std::string baseDir = newFileBaseDir.empty() ? dir : newFileBaseDir;
                    File f;
                    f.CreateFile(baseDir, rawName);

                    // select the new file in both context and persistent selection
                    fs::path createdPath = fs::path(baseDir) / rawName;
                    hasSelection = true;
                    selectedPath = createdPath.string();
                    selectedIsDir = false;
                    ctxHasSelection = true;
                    ctxIsDir = false;
                    ctxPath = createdPath.string();

                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            // New Folder popup (pre-creation naming)
            if (ImGui::BeginPopupModal("AssetsNewFolderPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextUnformatted("New folder name:");
                bool enterPressed = ImGui::InputText("##assets_new_folder_name", newFolderNameBuf, sizeof(newFolderNameBuf), ImGuiInputTextFlags_EnterReturnsTrue);

                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere(-1);
                }

                bool createRequested = false;
                if (ImGui::Button("Create##assets_new_folder") || enterPressed) createRequested = true;
                ImGui::SameLine();
                if (ImGui::Button("Cancel##assets_new_folder_cancel")) {
                    ImGui::CloseCurrentPopup();
                }

                if (createRequested) {
                    std::string rawName = newFolderNameBuf;
                    // trim spaces
                    while (!rawName.empty() && (rawName.back() == ' ' || rawName.back() == '\t'))
                        rawName.pop_back();
                    size_t p = 0;
                    while (p < rawName.size() && (rawName[p] == ' ' || rawName[p] == '\t'))
                        ++p;
                    rawName.erase(0, p);
                    if (rawName.empty()) rawName = "New Folder";

                    std::string baseDir = newFolderBaseDir.empty() ? dir : newFolderBaseDir;
                    File f;
                    f.CreateDirectory(baseDir, rawName);

                    // select the new folder in both context and persistent selection
                    fs::path createdPath = fs::path(baseDir) / rawName;
                    hasSelection = true;
                    selectedPath = createdPath.string();
                    selectedIsDir = true;
                    ctxHasSelection = true;
                    ctxIsDir = true;
                    ctxPath = createdPath.string();

                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            // Rename popup
            if (ImGui::BeginPopupModal("AssetsRenamePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextUnformatted("Rename to:");
                bool enterPressed = ImGui::InputText("##rename_name", renameBuf, sizeof(renameBuf), ImGuiInputTextFlags_EnterReturnsTrue);

                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere(-1);
                }

                if (ImGui::Button("Apply") || enterPressed) {
                    if (!renameTargetPath.empty()) {
                        fs::path p(renameTargetPath);
                        std::string parentDir = p.parent_path().string();
                        std::string oldName   = p.filename().string();
                        std::string newName   = renameBuf;
                        if (!newName.empty() && newName != oldName) {
                            File f;
                            f.RenameFile(parentDir, oldName, newName);
                            
                            // Update selection if the renamed item was selected
                            if (hasSelection && selectedPath == renameTargetPath) {
                                selectedPath = (fs::path(parentDir) / newName).string();
                            }
                        }
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel##rename")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

    if (Panels::showHierarchy) {
        ImGui::SetNextWindowBgAlpha(1.0f); 
        if (ImGui::Begin("Hierarchy", &Panels::showHierarchy)) {
            ImGui::Text("Placeholder");
        }
        ImGui::End();
    }

    if (Panels::showViewport) {
        // Always transparent regardless of docking
        
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        const ImGuiWindowFlags vp_flags = ImGuiWindowFlags_NoBackground;
        if (ImGui::Begin("Viewport", &Panels::showViewport, vp_flags)) {
            ImGui::Text("Placeholder");
            ImDrawList* dl = ImGui::GetWindowDrawList();

            // Anchor/scaling helpers
            ImVec2 winPos = ImGui::GetWindowPos();
            ImVec2 winSize = ImGui::GetWindowSize();
            ImVec2 windowTL = winPos;
            ImVec2 windowBR = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);
            ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
            ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
            ImVec2 contentTL = ImVec2(winPos.x + contentMin.x, winPos.y + contentMin.y);
            ImVec2 contentBR = ImVec2(winPos.x + contentMax.x, winPos.y + contentMax.y);
            ImVec2 contentSize = ImVec2(contentBR.x - contentTL.x, contentBR.y - contentTL.y);

            // Reference design size (matches original absolute coordinates)
            const ImVec2 designSize = ImVec2(1000.0f, 800.0f);
            float sx = contentSize.x / designSize.x;
            float sy = contentSize.y / designSize.y;
            float s = (sx < sy) ? sx : sy; // uniform scale to preserve aspect
            ImVec2 pad = ImVec2((contentSize.x - designSize.x * s) * 0.5f,
                                (contentSize.y - designSize.y * s) * 0.5f);

            auto T = [&](float x, float y) {
                return ImVec2(contentTL.x + pad.x + x * s, contentTL.y + pad.y + y * s);
            };

            // Determine if this window is snapped to the main viewport (app window) edges
            const ImGuiViewport* mainVP = ImGui::GetMainViewport();
            ImVec2 appTL = mainVP->Pos;
            ImVec2 appBR = ImVec2(mainVP->Pos.x + mainVP->Size.x, mainVP->Pos.y + mainVP->Size.y);
            const float edgeEpsilon = 2.0f; // tolerance for considering it snapped
            const bool leftSnapped = fabsf(windowTL.x - appTL.x) <= edgeEpsilon;
            const bool bottomSnapped = fabsf(windowBR.y - appBR.y) <= edgeEpsilon;
            const bool rightSnapped = fabsf(windowBR.x - appBR.x) <= edgeEpsilon;

            // Clip to app bounds only when extending to app edges; otherwise clip to window
            if (leftSnapped || bottomSnapped || rightSnapped) {
                dl->PushClipRect(appTL, appBR, true);
            } else {
                dl->PushClipRect(windowTL, windowBR, true);
            }

            
           

            // Gray L-shape hugging the left and bottom sides (constant thickness, unscaled)
            const float lThickness = 20.0f; // pixels
            const ImU32 lColor = IM_COL32(160, 160, 160, 255);
            // Left vertical bar (flush with window left edge, or app edge if snapped)
            float leftX0 = leftSnapped ? appTL.x : windowTL.x;
            float leftX1 = leftX0 + lThickness;
            dl->AddRectFilled(ImVec2(leftX0, windowTL.y),
                              ImVec2(leftX1, windowBR.y),
                              lColor, 0.0f);
            // Bottom horizontal bar (flush with window bottom edge, or app edge if snapped)
            float bottomY0 = bottomSnapped ? (appBR.y - lThickness) : (windowBR.y - lThickness);
            float bottomY1 = bottomSnapped ? appBR.y : windowBR.y;
            float bottomX0 = leftSnapped ? appTL.x : windowTL.x;
            float bottomX1 = rightSnapped ? appBR.x : windowBR.x;
            dl->AddRectFilled(ImVec2(bottomX0, bottomY0),
                              ImVec2(bottomX1, bottomY1),
                              lColor, 0.0f);

            // Inward-facing triangle indicators that track cursor position across the entire panel.
            // One per axis:
            // - Bottom edge: one up-pointing triangle follows mouse X within the panel width.
            // - Left edge: one right-pointing triangle follows mouse Y within the panel height.
            const ImU32 triColor = IM_COL32(50, 50, 50, 255);
            const float triSize = 20.0f;     // size of indicator triangles (pixels)
            const float edgePad = 3.0f;      // keep a small pad from bar edges when clamping

            ImVec2 mouse = ImGui::GetIO().MousePos;

            // Trackers persist across frames and freeze when cursor is out of the panel region
            static float trackX = 0.0f; // x-axis tracker for top/bottom indicators
            static float trackY = 0.0f; // y-axis tracker for left indicators
            static bool trackXInit = false;
            static bool trackYInit = false;

            // Initialize trackers to center if not set yet
            if (!trackXInit) { trackX = (windowTL.x + windowBR.x) * 0.5f; trackXInit = true; }
            if (!trackYInit) { trackY = (windowTL.y + windowBR.y) * 0.5f; trackYInit = true; }

            // Entire panel region for tracking
            const float pMinX = windowTL.x + edgePad;
            const float pMaxX = windowBR.x - edgePad;
            const float pMinY = windowTL.y + edgePad;
            const float pMaxY = windowBR.y - edgePad;
            const bool mouseInPanel = (mouse.x >= pMinX && mouse.x <= pMaxX && mouse.y >= pMinY && mouse.y <= pMaxY);
            if (mouseInPanel) {
                // Update trackers while within panel
                trackX = (mouse.x < pMinX) ? pMinX : (mouse.x > pMaxX ? pMaxX : mouse.x);
                trackY = (mouse.y < pMinY) ? pMinY : (mouse.y > pMaxY ? pMaxY : mouse.y);
            }

            // Draw one inward-facing triangle on the bottom edge at X = trackX (upward)
            {
                float apexY = bottomY0 + edgePad;       // tip near inner edge of bar (top of the bar)
                float baseY = bottomY1 - edgePad;       // base near bottom edge of bar
                float halfW = triSize * 0.5f;
                // clamp X so the full base fits within panel
                float minX = (bottomX0 + edgePad) + halfW;
                float maxX = (bottomX1 - edgePad) - halfW;
                float cx = trackX < minX ? minX : (trackX > maxX ? maxX : trackX);

                ImVec2 b_a = ImVec2(cx, apexY);                // apex (points inward)
                ImVec2 b_b = ImVec2(cx - halfW, baseY);
                ImVec2 b_c = ImVec2(cx + halfW, baseY);
                dl->AddTriangleFilled(b_a, b_b, b_c, triColor);
            }

            // Draw one inward-facing triangle on the left edge at Y = trackY (rightward)
            {
                float apexX = leftX1 - edgePad;         // tip near inner edge of bar
                float baseX = leftX0 + edgePad;         // base near outer edge of bar
                float halfH = triSize * 0.5f;
                const float vMinY = windowTL.y + edgePad + halfH; // clamp so full triangle fits
                const float vMaxY = windowBR.y - edgePad - halfH;
                float cy = (trackY < vMinY) ? vMinY : (trackY > vMaxY ? vMaxY : trackY);

                ImVec2 l_a = ImVec2(apexX, cy);                 // apex (points inward)
                ImVec2 l_b = ImVec2(baseX, cy - halfH);
                ImVec2 l_c = ImVec2(baseX, cy + halfH);
                dl->AddTriangleFilled(l_a, l_b, l_c, triColor);
            }

            dl->PopClipRect();
        }
        ImGui::End();
        ImGui::PopStyleColor(1);
        
    }

    if (Panels::showGameView) {
        ImGui::SetNextWindowBgAlpha(0.0f); 
        if (ImGui::Begin("Game View", &Panels::showGameView)) {
            ImGui::Text("Placeholder");
        }
        ImGui::End();
    }


   




    if (Panels::showInspector) {
        if (ImGui::Begin("Inspector", &Panels::showInspector)) {
            // Full-panel drag-and-drop target without affecting layout/scrolling
            {
                ImGuiWindow* w = ImGui::GetCurrentWindow();
                if (w != nullptr) {
                    ImRect target = w->InnerRect;
                    if (ImGui::BeginDragDropTargetCustom(target, w->ID)) {
                        // Accept both script and general file payloads (but only process .cs files)
                        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("OMNIX_SCRIPT_PATH");
                        if (!payload) {
                            payload = ImGui::AcceptDragDropPayload("OMNIX_FILE_PATH");
                        }
                        
                        if (payload) {
                            const char* droppedPath = reinterpret_cast<const char*>(payload->Data);
                            if (droppedPath) {
                                fs::path p(droppedPath);
                                // Only process .cs files in Inspector
                                if (p.extension() == ".cs") {
                                    ScriptState& st = scriptState();
                                    st.currentScriptPath = droppedPath;
                                    st.currentScriptName = p.filename().string();
                                    st.loaded = true;
                                    st.awaitingSave = false;
                                    std::error_code ec;
                                    if (fs::exists(st.currentScriptPath, ec)) {
                                        st.lastWriteTime = fs::last_write_time(st.currentScriptPath, ec);
                                    }
                                }
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                }
            }

            // Creation popup (triggered by + button at bottom)
            if (ImGui::BeginPopupModal("Create C# Script", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ScriptState& st = scriptState();
                ImGui::TextUnformatted("Script name:");
                ImGui::SetNextItemWidth(260.0f);
                ImGui::InputText("##script_name", st.nameBuf, sizeof(st.nameBuf));

                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere(-1);
                }

                bool createRequested = false;
                if (ImGui::Button("Create")) createRequested = true;
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); }

                if (createRequested) {
                    std::string rawName = st.nameBuf;
                    // trim spaces
                    while (!rawName.empty() && (rawName.back() == ' ' || rawName.back() == '\t')) rawName.pop_back();
                    size_t p = 0; while (p < rawName.size() && (rawName[p] == ' ' || rawName[p] == '\t')) ++p; rawName.erase(0, p);
                    if (rawName.empty()) rawName = "NewScript";
                    // handle extension and class name
                    std::string fileName = ensureCSExt(rawName);
                    std::string className = sanitizeClassName(rawName);
                    if (fileName.size() >= 3 && fileName.rfind(".cs") == fileName.size() - 3) {
                        className = sanitizeClassName(fileName.substr(0, fileName.size() - 3));
                    }

                    std::string dir = ensureScriptsDir();
                    std::string path = dir + "/" + fileName;

                    // Create file if it doesn't exist, else keep existing
                    std::error_code ec;
                    bool exists = fs::exists(path, ec);
                    if (!exists) {
                        if (!writeCSharpTemplate(path, className)) {
                            // Failed to create; keep popup open
                        } else {
                            exists = true;
                        }
                    }
                    if (exists) {
                        st.currentScriptName = fileName;
                        st.currentScriptPath = path;
                        st.loaded = false;
                        st.awaitingSave = true;
                        std::error_code ec2;
                        if (fs::exists(path, ec2)) {
                            st.lastWriteTime = fs::last_write_time(path, ec2);
                        }
                        tryOpenWithVSCode(path);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }

            // Poll for script save if awaiting
            {
                ScriptState& st = scriptState();
                if (st.awaitingSave && !st.currentScriptPath.empty()) {
                    std::error_code ec;
                    if (fs::exists(st.currentScriptPath, ec)) {
                        fs::file_time_type nowWT = fs::last_write_time(st.currentScriptPath, ec);
                        if (!ec && nowWT > st.lastWriteTime) {
                            st.loaded = true;
                            st.awaitingSave = false;
                            st.lastWriteTime = nowWT;
                        }
                    }
                }
            }

            // Loaded box with reopen action
            {
                ScriptState& st = scriptState();
                if (st.loaded && !st.currentScriptName.empty()) {
                    ImGui::Separator();
                    std::string label = st.currentScriptName + " is loaded";
                    float fullw = ImGui::GetContentRegionAvail().x;
                    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(fullw, 0.0f))) {
                        tryOpenWithVSCode(st.currentScriptPath);
                    }
                }
            }

            // Centered + button locked to bottom, larger and rounded
            {
                const float btnW = 240.0f;
                const float btnH = 33.0f;
                ImVec2 savedCursor = ImGui::GetCursorScreenPos();
                ImVec2 winPos = ImGui::GetWindowPos();
                ImVec2 crMin = ImGui::GetWindowContentRegionMin();
                ImVec2 crMax = ImGui::GetWindowContentRegionMax();
                ImVec2 contentTL = ImVec2(winPos.x + crMin.x, winPos.y + crMin.y);
                ImVec2 contentBR = ImVec2(winPos.x + crMax.x, winPos.y + crMax.y);
                float x = contentTL.x + ((contentBR.x - contentTL.x) - btnW) * 0.5f;
                float y = contentBR.y - btnH;
                ImGui::SetCursorScreenPos(ImVec2(x, y));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
                // Draw background via button with empty label, then overlay centered '+'
                if (ImGui::Button("##add_btn", ImVec2(btnW, btnH))) {
                    ScriptState& st = scriptState();
                    st.loaded = false;
                    st.awaitingSave = false;
                    st.currentScriptName.clear();
                    st.currentScriptPath.clear();
                    st.nameBuf[0] = '\0';
                    ImGui::OpenPopup("Create C# Script");
                }
                // Overlay centered '+' manually
                {
                    ImVec2 rmin = ImGui::GetItemRectMin();
                    ImVec2 rmax = ImGui::GetItemRectMax();
                    const char* plus = "+";
                    ImVec2 ts = ImGui::CalcTextSize(plus);
                    ImVec2 tp = ImVec2(rmin.x + (rmax.x - rmin.x - ts.x) * 0.5f,
                                        rmin.y + (rmax.y - rmin.y - ts.y) * 0.5f);
                    ImGui::GetWindowDrawList()->AddText(tp, ImGui::GetColorU32(ImGuiCol_Text), plus);
                }
                ImGui::PopStyleVar();
                // Restore cursor so subsequent UI is not shifted down
                ImGui::SetCursorScreenPos(savedCursor);
            }

    // Per-axis Unity-like label drags + numeric inputs
    const char* axisLabels[3] = {"X", "Y", "Z"};
    const ImU32 axisBg[3] = { IM_COL32(170, 60, 60, 200), IM_COL32(60, 170, 60, 200), IM_COL32(60, 120, 200, 200) };
    const ImU32 axisBorder = IM_COL32(40, 40, 40, 255);
    const ImGuiStyle& style = ImGui::GetStyle();
    for (int i = 0; i < 3; ++i) {
        // Label box sized to frame height with centered text and colored background
        const float labelW = 26.0f;
        const float labelH = ImGui::GetFrameHeight();
        ImVec2 labelPos = ImGui::GetCursorScreenPos();
        ImVec2 labelBR  = ImVec2(labelPos.x + labelW, labelPos.y + labelH);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        float rounding = style.FrameRounding;
        dl->AddRectFilled(labelPos, labelBR, axisBg[i], rounding);
        dl->AddRect(labelPos, labelBR, axisBorder, rounding, 0, 1.0f);
        RenderCenteredTextInRect(labelPos, labelBR, axisLabels[i]);

        // Make the label box interactive for drag
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos(labelPos);
        ImGui::InvisibleButton("##axis_label_drag", ImVec2(labelW, labelH));
        if (ImGui::IsItemActive()) {
            ImGuiIO& io = ImGui::GetIO();
            float speed = 0.01f;
            float delta = io.MouseDelta.x * speed;
            scale[i] += delta;
        }
        ImGui::PopID();

        // Move to input on same line, aligned to frame height
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        char buf[16];
        snprintf(buf, sizeof(buf), "##val_%d", i);
        ImGui::InputFloat(buf, &scale[i], 0.01f, 0.1f, "%.3f");
    }

        }
        ImGui::End();
    }




}

void UI::setProjectName(const std::string& name) {
    g_projectName = name;
}


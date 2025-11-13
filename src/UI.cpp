#include "UI.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Panels.h"
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

    struct ScriptState {
        std::string scriptsDir;
        std::string currentScriptPath;
        std::string currentScriptName; // with .cs
        fs::file_time_type lastWriteTime{};
        bool awaitingSave = false;
        bool loaded = false;
        char nameBuf[128] = {0};
    };

    ScriptState& scriptState() {
        static ScriptState s;
        return s;
    }

    std::string getHomeDir() {
        const char* home = std::getenv("HOME");
        return home ? std::string(home) : std::string("/");
    }

    std::string ensureScriptsDir() {
        ScriptState& st = scriptState();
        if (!st.scriptsDir.empty()) return st.scriptsDir;
        std::string dir = getHomeDir() + "/Library/Application Support/Omnix/Scripts";
        std::error_code ec;
        fs::create_directories(dir, ec);
        st.scriptsDir = dir;
        return st.scriptsDir;
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
            // List all .cs scripts as icons with names, double-click to open, draggable to Inspector
            ScriptState& st = scriptState();
            std::string dir = ensureScriptsDir();

            std::vector<std::pair<std::string, std::string>> scripts; // {name, path}
            {
                std::error_code ec;
                if (fs::exists(dir, ec)) {
                    for (auto it = fs::directory_iterator(dir, ec); !ec && it != fs::end(it); it.increment(ec)) {
                        if (ec) break;
                        const fs::directory_entry& de = *it;
                        if (!de.is_regular_file(ec)) continue;
                        const fs::path& p = de.path();
                        if (p.extension() == ".cs") {
                            scripts.emplace_back(p.filename().string(), p.string());
                        }
                    }
                }
            }
            std::sort(scripts.begin(), scripts.end(), [](auto& a, auto& b){ return a.first < b.first; });

            // Grid settings
            const float iconSize = 36.0f;
            const float labelHeight = ImGui::GetTextLineHeight();
            const float cellWidth = 120.0f;
            const float cellHeight = iconSize + 6.0f + labelHeight + 8.0f;
            const float availX = ImGui::GetContentRegionAvail().x;
            int cols = (int)std::max(1.0f, floorf(availX / cellWidth));

            int col = 0;
            for (auto& [name, path] : scripts) {
                if (col > 0) ImGui::SameLine();
                ImGui::BeginGroup();

                // Icon button styled as a filled square with "CS"
                ImGui::PushID(path.c_str());
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                // Center icon horizontally within the cell
                float iconX = p0.x + (cellWidth - iconSize) * 0.5f;
                ImVec2 ip0 = ImVec2(iconX, p0.y);
                ImVec2 ip1 = ImVec2(iconX + iconSize, p0.y + iconSize);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const ImU32 iconBg = IM_COL32(60, 120, 200, 255);
                const ImU32 iconBorder = IM_COL32(30, 60, 100, 255);
                dl->AddRectFilled(ip0, ip1, iconBg, 6.0f);
                dl->AddRect(ip0, ip1, iconBorder, 6.0f, 0, 2.0f);
                ImGui::SetCursorScreenPos(ip0);
                ImGui::InvisibleButton("icon", ImVec2(iconSize, iconSize));

                bool iconDoubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0);

                // Drag source from the icon
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("OMNIX_SCRIPT_PATH", path.c_str(), path.size() + 1);
                    ImGui::TextUnformatted(name.c_str());
                    ImGui::EndDragDropSource();
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
                    tryOpenWithVSCode(path);
                }

                // Also allow dragging by the label area
                ImGui::SetItemAllowOverlap();
                ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + iconSize + 6.0f));
                ImGui::InvisibleButton("lbl", ImVec2(cellWidth, labelHeight + 8.0f));
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    tryOpenWithVSCode(path);
                }
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("OMNIX_SCRIPT_PATH", path.c_str(), path.size() + 1);
                    ImGui::TextUnformatted(name.c_str());
                    ImGui::EndDragDropSource();
                }

                ImGui::PopID();
                ImGui::EndGroup();

                col = (col + 1) % cols;
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
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("OMNIX_SCRIPT_PATH")) {
                            const char* droppedPath = reinterpret_cast<const char*>(payload->Data);
                            if (droppedPath) {
                                ScriptState& st = scriptState();
                                st.currentScriptPath = droppedPath;
                                fs::path p(droppedPath);
                                st.currentScriptName = p.filename().string();
                                st.loaded = true;
                                st.awaitingSave = false;
                                std::error_code ec;
                                if (fs::exists(st.currentScriptPath, ec)) {
                                    st.lastWriteTime = fs::last_write_time(st.currentScriptPath, ec);
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



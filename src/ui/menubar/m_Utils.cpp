#include "ui/menubar/m_Utils.hpp"

void MenuBar::changeWindowVisibility(const std::filesystem::path& windowName, bool* isOpen) {
    if (isOpen == nullptr) {
        LOG("ERROR", "Ponteiro nulo ao tentar acessar a visibilidade da janela '" + windowName.string() + "'.");
        return;
    }

    if (ImGui::MenuItem(windowName.string().c_str(), nullptr, *isOpen)) {
        *isOpen             = !(*isOpen);
        std::string message = *isOpen ? "Foi aberta a janela '" + windowName.string() + "'."
                                      : "Foi fechada a janela '" + windowName.string() + "'.";
        LOG("TRACE", message);
    }
}

void MenuBar::renderStatus() {
    // Get Hour
    std::time_t t   = std::time(nullptr);
    std::tm*    now = std::localtime(&t);
    char        currentTime[64];
    std::strftime(currentTime, sizeof(currentTime), "%H:%M:%S  %d-%m-%Y", now);

    // Get FPS
    float fps = ImGui::GetIO().Framerate;

    char status[128];
    std::snprintf(status, sizeof(status), "%.1f  %s", fps, currentTime);
    float statusWidth = ImGui::CalcTextSize(status)[0];
    ImGui::SameLine(ImGui::GetWindowWidth() - statusWidth * 1.1f);
    ImGui::Text("%s", status);
}

void MenuBar::renderProgramName() {
    std::string programName = "Fórmula Tesla";
    float       windowWidth = ImGui::GetWindowWidth();
    float       textWidth   = ImGui::CalcTextSize(programName.c_str())[0];
    float       textOffsetX = (windowWidth - textWidth) / 2.0f;
    ImGui::SetCursorPosX(textOffsetX);
    ImGui::Text("%s", programName.c_str());
}

void MenuBar::changeColorMap() {
    if (ImGui::BeginMenu("Mudar Cores")) {
        ImPlotContext&  gp       = *GImPlot;
        ImPlotColormap& colormap = gp.Style.Colormap;

        if (ImPlot::ColormapButton(ImPlot::GetColormapName(colormap), ImVec2(225, 0), colormap)) {
            colormap = (colormap + 1) % ImPlot::GetColormapCount();
            ImPlot::BustItemCache();
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImPlot::ShowColormapSelector("##");

        ImGui::EndMenu();
    }
}

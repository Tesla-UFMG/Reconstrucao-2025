#include "ui/menubar/m_Utils.hpp"
#include "ui/windows/w_Telemetry.hpp"

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
    int telemetryStatus  = DB::getInstance().getProject().getTelemetryStatus();
    bool processingStatus = DB::getInstance().getProject().getProcessingStatus();

    std::string statusWord;
    std::string procWord;
    ImVec4 statusColor;
    ImVec4 procColor;

    if (telemetryStatus == 1) {
        statusWord = "Conectado  ";
        statusColor = HI(1.0f);
        procWord = processingStatus ? "Ok  " : "Erro  ";
        procColor = processingStatus ? HI(1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    } else if (telemetryStatus == 2) {
        statusWord = "Reconectando...  ";
        statusColor = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
    } else {
        statusWord = "Desconectado  ";
        statusColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    }

    std::string restText;
    // Get FPS
    float fps = ImGui::GetIO().Framerate;
    char  fpsText[16];
    std::snprintf(fpsText, sizeof(fpsText), "%.1f  ", fps);
    restText += fpsText;

    // Get Hour
    std::time_t t   = std::time(nullptr);
    std::tm*    now = std::localtime(&t);
    char        currentTime[64];
    std::strftime(currentTime, sizeof(currentTime), "%H:%M:%S  %d-%m-%Y", now);
    restText += currentTime;

    // Render text
    float totalWidth = ImGui::CalcTextSize(statusWord.c_str()).x;
    if (!procWord.empty()) {
        totalWidth += ImGui::CalcTextSize(procWord.c_str()).x;
    }
    totalWidth += ImGui::CalcTextSize(restText.c_str()).x;
    ImGui::SameLine(ImGui::GetWindowWidth() - totalWidth * 1.1f);

    ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
    ImGui::Text("%s", statusWord.c_str());
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) {
        ImGui::SetItemTooltip("Clique para conectar ou desconectar a UART");
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        if (auto telemetryWin = Window::Telemetry::getInstance()) {
            telemetryWin->toggleConnection();
        }
    }

    if (!procWord.empty()) {
        ImGui::SameLine(0, 0);
        ImGui::PushStyleColor(ImGuiCol_Text, procColor);
        ImGui::Text("%s", procWord.c_str());
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) {
            ImGui::SetItemTooltip("Clique para conectar ou desconectar a UART");
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            if (auto telemetryWin = Window::Telemetry::getInstance()) {
                telemetryWin->toggleConnection();
            }
        }
    }

    ImGui::SameLine(0, 0);
    ImGui::Text("%s", restText.c_str());
}

void MenuBar::renderProgramName() {
    std::string programName = "Fórmula Tesla";
    float       windowWidth = ImGui::GetWindowWidth();
    float       textWidth   = ImGui::CalcTextSize(programName.c_str())[0];
    float       textOffsetX = (windowWidth - textWidth) / 2.0f;
    ImGui::SetCursorPosX(textOffsetX);
    ImGui::Text("%s", programName.c_str());
}

void MenuBar::changePlotColormap() {
    if (ImGui::BeginMenu("Mudar Cores")) {
        ImPlotContext&  gp       = *GImPlot;
        ImPlotColormap& colormap = gp.Style.Colormap;

        if (ImPlot::ColormapButton(ImPlot::GetColormapName(colormap), ImVec2(225, 0), colormap)) {
            colormap = (colormap + 1) % ImPlot::GetColormapCount();
            ImGuiWrapper::saveAppTheme();
            ImPlot::BustItemCache();
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImPlot::ShowColormapSelector("##");

        ImGui::EndMenu();
    }
}

void MenuBar::changeAppStyleTheme() {
    if (ImGui::BeginMenu("Mudar Tema")) {

        if (ImGui::MenuItem("Escuro")) {
            ImGuiWrapper::changeStyleTheme(DARK);
        }

        if (ImGui::MenuItem("Claro")) {
            ImGuiWrapper::changeStyleTheme(LIGHT);
        }

        ImGui::EndMenu();
    }
}
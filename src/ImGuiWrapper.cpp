#include "ImGuiWrapper.hpp"

bool                  ImGuiWrapper::isSubsystemInited = false;
ImGuiIO*              ImGuiWrapper::io                = nullptr;
std::filesystem::path ImGuiWrapper::layoutQueue;

void ImGuiWrapper::initSubsystem() {
    if (ImGuiWrapper::isSubsystemInited) {
        LOG("WARN", "ImGui já está iniciado. Não é possível iniciá-lo novamente.");
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiWrapper::io = &ImGui::GetIO();
    (void)ImGuiWrapper::io; // Documentação pede para fazer esse cast
    ImGuiWrapper::io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGuiWrapper::io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGuiWrapper::configStyle();

    ImGui_ImplSDL2_InitForSDLRenderer(SDLWrapper::window, SDLWrapper::renderer);
    ImGui_ImplSDLRenderer2_Init(SDLWrapper::renderer);
    ImGuiWrapper::isSubsystemInited = true;
    LOG("TRACE", "ImGui foi iniciado com sucesso.");
}

void ImGuiWrapper::prepareForNewFrame() {
    ImGuiWrapper::loadLayoutFromQueue();
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ImGuiWrapper::render() {
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), SDLWrapper::renderer);
}

void ImGuiWrapper::closeSubystem() {
    if (ImGuiWrapper::isSubsystemInited == false) {
        LOG("WARN", "ImGui já está fechado. Não é possível fechá-lo novamente.");
        return;
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    ImGuiWrapper::isSubsystemInited = false;
    LOG("TRACE", "ImGui encerrado.");
}

void ImGuiWrapper::saveLayout(const std::filesystem::path& filepath) {
    size_t      size;
    const char* iniData = ImGui::SaveIniSettingsToMemory(&size);

    std::filesystem::path parentPath = filepath.parent_path();
    if (!parentPath.empty() && std::filesystem::create_directories(parentPath)) {
        LOG("INFO", "Criada pasta '" + parentPath.string() + "'.");
    }

    std::ofstream outFile(filepath, std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(iniData, size);
        outFile.close();
        LOG("INFO", "Layout '" + filepath.string() + "' salvo com sucesso.");
    } else {
        LOG("WARN", "Não foi possível salvar o layout '" + filepath.string() + "'.");
    }
}

void ImGuiWrapper::loadLayoutFromQueue() {
    if (ImGuiWrapper::layoutQueue.empty()) {
        return;
    }

    std::filesystem::path filepath = ImGuiWrapper::layoutQueue;
    ImGuiWrapper::layoutQueue.clear();

    std::ifstream inFile(filepath, std::ios::binary);
    if (inFile.is_open()) {
        std::string iniData, line;
        while (std::getline(inFile, line))
            iniData += line + '\n';

        ImGui::LoadIniSettingsFromMemory(iniData.c_str(), iniData.size());
        inFile.close();

        LOG("INFO", "Layout '" + filepath.string() + "' carregado com sucesso.");
    } else {
        LOG("WARN", "Não foi possível carregar o layout '" + filepath.string() + "'.");
    }
}

void ImGuiWrapper::loadLayout(const std::filesystem::path& filepath) { ImGuiWrapper::layoutQueue = filepath; }

void ImGuiWrapper::handleEvent(SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    if (event.type == SDL_KEYDOWN) {
        int offset = 1073741881;
        if (event.key.keysym.sym > offset &&
            event.key.keysym.sym < offset + 11) { // Entre F1 e F10. Olhe no SDL_keycode
            std::string F_number = std::to_string(event.key.keysym.sym - offset);
            if ((event.key.keysym.mod & KMOD_CTRL)) { // Se CTRL estiver precionado...
                Window::saveWindowVisibility("./data/layouts/.visibility_" + F_number);
                ImGuiWrapper::saveLayout("./data/layouts/.layout_" + F_number);
            } else {
                Window::loadWindowVisibility("./data/layouts/.visibility_" + F_number);
                ImGuiWrapper::loadLayout("./data/layouts/.layout_" + F_number);
            }
        }
    }
}

void ImGuiWrapper::configStyle() {
    ImGuiStyle& style  = ImGui::GetStyle();
    ImVec4*     colors = style.Colors;

    // Text
    colors[ImGuiCol_Text]           = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = HI(0.35f);

    // BG
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg]  = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]  = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);

    // Border
    colors[ImGuiCol_Border]       = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Frames
    colors[ImGuiCol_FrameBg]        = LOW(0.65f);
    colors[ImGuiCol_FrameBgHovered] = HI(0.4f);
    colors[ImGuiCol_FrameBgActive]  = HI(0.67f);

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    // colors[ImGuiCol_TitleBgActive]    = ImVec4(0.082f, 0.509f, 0.231f, 1.00f);
    colors[ImGuiCol_TitleBgActive]    = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]        = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);

    // Scroll bar
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);

    colors[ImGuiCol_CheckMark] = HI(1.0f);

    // Slider
    colors[ImGuiCol_SliderGrab]       = HI(0.8f);
    colors[ImGuiCol_SliderGrabActive] = HI(1.0f);

    // Button
    colors[ImGuiCol_Button]        = HI(0.4f);
    colors[ImGuiCol_ButtonHovered] = HI(0.8f);
    colors[ImGuiCol_ButtonActive]  = HI(1.0f);

    // Header
    colors[ImGuiCol_Header]        = HI(0.31f);
    colors[ImGuiCol_HeaderHovered] = HI(0.8f);
    colors[ImGuiCol_HeaderActive]  = HI(1.0f);

    colors[ImGuiCol_TextLink] = colors[ImGuiCol_HeaderActive];

    // Separator
    colors[ImGuiCol_Separator]        = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = HI(0.67f);
    colors[ImGuiCol_SeparatorActive]  = HI(0.95f);

    // Resize
    colors[ImGuiCol_ResizeGrip]        = HI(0.2f);
    colors[ImGuiCol_ResizeGripHovered] = HI(0.67f);
    colors[ImGuiCol_ResizeGripActive]  = HI(0.95f);

    // Tabs
    colors[ImGuiCol_TabHovered]          = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab]                 = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabSelected]         = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed]           = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected]   = ImLerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);

    // Docking
    colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

    // Plot
    colors[ImGuiCol_PlotLines]            = ImVec4(0.860f, 0.930f, 0.890f, 0.80f);
    colors[ImGuiCol_PlotLinesHovered]     = ImVec4(0.860f, 0.930f, 0.890f, 0.95f);
    colors[ImGuiCol_PlotHistogram]        = ImVec4(0.860f, 0.930f, 0.890f, 0.80f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.860f, 0.930f, 0.890f, 0.95f);

    // Table
    colors[ImGuiCol_TableHeaderBg]     = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]  = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]     = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);

    // Misc
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]             = HI(1.0f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
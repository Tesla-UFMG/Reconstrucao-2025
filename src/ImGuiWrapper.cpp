#include "ImGuiWrapper.hpp"

bool     ImGuiWrapper::isSubsystemInited = false;
ImGuiIO* ImGuiWrapper::io                = nullptr;

void ImGuiWrapper::initSubsystem() {
    if (ImGuiWrapper::isSubsystemInited) {
        Log::getInstance().message("WARN", "ImGui já está iniciado. Não é possível iniciá-lo novamente.");
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiWrapper::io = &ImGui::GetIO();
    (void)ImGuiWrapper::io; // Documentação pede para fazer esse cast
    ImGuiWrapper::io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGuiWrapper::io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(SDLWrapper::window, SDLWrapper::renderer);
    ImGui_ImplSDLRenderer2_Init(SDLWrapper::renderer);
    ImGuiWrapper::isSubsystemInited = true;
    Log::getInstance().message("TRACE", "ImGui foi iniciado com sucesso.");
}

void ImGuiWrapper::prepareForNewFrame() {
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
        Log::getInstance().message("WARN", "ImGui já está fechado. Não é possível fechá-lo novamente.");
        return;
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    ImGuiWrapper::isSubsystemInited = false;
    Log::getInstance().message("TRACE", "ImGui encerrado.");
}
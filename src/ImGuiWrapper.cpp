#include "ImGuiWrapper.hpp"

bool ImGuiWrapper::isSubsystemInited = false;
ImGuiIO* ImGuiWrapper::io = nullptr;

void ImGuiWrapper::initSubsystem(){
    if (ImGuiWrapper::isSubsystemInited) return;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiWrapper::io = &ImGui::GetIO(); (void) ImGuiWrapper::io; // Documentação pede para fazer esse cast 
    ImGuiWrapper::io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 
    ImGuiWrapper::io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;   

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(SDLWrapper::window, SDLWrapper::renderer);
    ImGui_ImplSDLRenderer2_Init(SDLWrapper::renderer);
    ImGuiWrapper::isSubsystemInited = true;
}

void ImGuiWrapper::prepareForNewFrame(){
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ImGuiWrapper::render(){
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), SDLWrapper::renderer);
}

void ImGuiWrapper::closeSubystem(){
    if (ImGuiWrapper::isSubsystemInited == false) return;
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    ImGuiWrapper::isSubsystemInited = false;
}
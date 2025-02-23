#ifndef IMGUIWRAPPER_HPP
#define IMGUIWRAPPER_HPP

// C++
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Project
#include "Log.hpp"
#include "SDLWrapper.hpp"

// Third party
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui_internal.h>
#include <implot.h>
#include <implot_internal.h>

#define HI(a)  ImVec4(0.113f, 0.725f, 0.329f, a)
#define MED(a) ImVec4(0.184f, 0.564f, 0.317f, a)
#define LOW(a) ImVec4(0.152f, 0.231f, 0.180f, a)

class ImGuiWrapper {
    private:
        static bool                  isSubsystemInited;
        static ImGuiIO*              io;
        static std::filesystem::path layoutQueue;
        static void                  configStyle();
        static void                  loadLayoutFromQueue();

    public:
        static void initSubsystem();      // Inicia todos os subsistemas do ImGui
        static void prepareForNewFrame(); // Prepara o ImGui para um novo frame
        static void render();             // Renderiza as janelas do ImGui
        static void closeSubystem();      // Fecha todos os subsistemas do ImGui
        static void handleEvent(SDL_Event& event);
        static void saveLayout(const std::filesystem::path& filepath);
        static void loadLayout(const std::filesystem::path& filepath);
};

#endif
#ifndef IWINDOW_HPP
#define IWINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"

class IWindow {
    protected:
        ImGuiWindowFlags flags;
        bool*            isOpen = nullptr;

    public:
        virtual void setupVisibility(bool* isOpen);
        virtual void render() = 0;
};

#endif
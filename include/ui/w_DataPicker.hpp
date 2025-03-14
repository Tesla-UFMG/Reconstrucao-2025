#ifndef DATAPICKER_WINDOW_HPP
#define DATAPICKER_WINDOW_HPP

// Project
#include "App.hpp"
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/iWindow.hpp"

// Third Party
#include "rapidcsv.h"
#include "tinyfiledialogs.h"

namespace Window {
    namespace MenuBar {
        void Datapicker();
    }

    void Datapicker(bool* isOpen);
} // namespace Window

#endif
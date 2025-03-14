#ifndef DATAPICKER_WINDOW_HPP
#define DATAPICKER_WINDOW_HPP

// Project
#include "App.hpp"
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

// Third Party
#include "rapidcsv.h"
#include "tinyfiledialogs.h"

namespace Window {
    class DataPicker : public IWindow {
        public:
            explicit DataPicker(bool* isOpen = nullptr);
            void         MenuBar();
            virtual void render() override;
    };

} // namespace Window

#endif
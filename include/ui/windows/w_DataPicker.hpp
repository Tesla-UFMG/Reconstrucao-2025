#ifndef DATAPICKER_WINDOW_HPP
#define DATAPICKER_WINDOW_HPP

// Project
#include "App.hpp"
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

// C++
#include <filesystem>
#include <string>

// Third Party
#include "rapidcsv.h"
#include "tinyfiledialogs.h"

namespace Window {
    class DataPicker : public IWindow {
        private:
            void refreshData();
            void sendArchivePayload(const std::string& filepath);
            void sendColumnPayload(const std::string& filepath, const std::string& columnName);

            void renderMenuBar();
            void renderArchiveContextPopup(const GenericFile& file);
            void renderArchiveNode(const GenericFile& file, int index);
            void renderColumnItem(const std::string& filepath, const std::string& colName);

        public:
            explicit DataPicker(bool* isOpen = nullptr);
            void render() override;
    };

} // namespace Window

#endif
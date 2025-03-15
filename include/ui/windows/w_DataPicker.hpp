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
            std::vector<std::filesystem::path>    paths;
            std::vector<std::vector<std::string>> columns;

            void refreshData();
            void sendArchivePayload(const std::string& filename);
            void sendColumnPayload(const std::string& filename, const std::string& columnName);

            void renderMenuBar();
            void renderArchiveContextPopup(const std::filesystem::path& archivePath);
            void renderArchiveNode(const std::filesystem::path& archivePath, size_t i);
            void renderColumnItem(const std::string& filename, const std::string& colName);

        public:
            explicit DataPicker(bool* isOpen = nullptr);
            void render() override;
    };

} // namespace Window

#endif
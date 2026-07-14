#ifndef DIALOGS_HPP
#define DIALOGS_HPP

// Project
#include "Log.hpp"

// C++
#include <string>
#include <vector>

// Third Party
#include "tinyfiledialogs.h"

class Dialogs {
    public:
        static char* showSaveFileDialog(const std::string& title, const std::string& defaultName, const char* filter);
        static char* showOpenFileDialog(const std::string& title, const std::vector<const char*>& filters, const char* description);
        static void  showErrorDialog(const std::string& message);
        static bool  showConfirmationDialog(const std::string& message);
        static std::string showInputDialog(const std::string& title, const std::string& message,
                                           const std::string& defaultInput = "");
};

#endif // DIALOGS_HPP
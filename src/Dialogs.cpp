#include "Dialogs.hpp"

char* Dialogs::showSaveFileDialog(const std::string& title, const std::string& defaultName, const char* filter) {
    const char* filters[] = {filter, nullptr};
    char*       filepath  = tinyfd_saveFileDialog(title.c_str(), ("./" + defaultName).c_str(), 1, filters, filter);
    if (!filepath) {
        LOG("ERROR", "Não foi selecionado nenhum local de salvamento.");
        return nullptr;
    }
    return filepath;
}

char* Dialogs::showOpenFileDialog(const std::string& title, const char* filter) {
    const char* filters[] = {filter, nullptr};
    char*       filepath  = tinyfd_openFileDialog(title.c_str(), "./", 1, filters, filter, 0);
    if (!filepath) {
        LOG("ERROR", "Não foi selecionado nenhum arquivo.");
        return nullptr;
    }
    return filepath;
}

void Dialogs::showErrorDialog(const std::string& message) {
    tinyfd_messageBox("Erro", message.c_str(), "ok", "error", 1);
}

bool Dialogs::showConfirmationDialog(const std::string& message) {
    return tinyfd_messageBox("Confirmação", message.c_str(), "yesno", "question", 1) == 1;
}

std::string Dialogs::showInputDialog(const std::string& title, const std::string& message,
                                     const std::string& defaultInput) {
    return tinyfd_inputBox(title.c_str(), message.c_str(), defaultInput.c_str());
}
// Em src/ui/windows/w_Playback.hpp

#ifndef PLAYBACK_WINDOW_HPP
#define PLAYBACK_WINDOW_HPP

#include "ui/windows/iWindow.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/windows/IPlayable.hpp"
#include <vector>
#include <map>
#include <string>
#include <set> // -> Adicionado para usar conjuntos

namespace Window {

    class Playback : public IWindow {
        public:
            Playback(bool* isOpen, const std::vector<IPlayable*>& playables);
            void render() override;

        private:
            // --- Métodos Privados Auxiliares ---
            void RenderGroupControls();
            void RenderGroupCreationUI();
            void RenderIndividualControls();

            // --- Atributos de Estado ---
            std::vector<IPlayable*> m_playables;

            // Estrutura para os grupos: um mapa com nome do grupo e um conjunto de ponteiros para os playbacks
            std::map<std::string, std::set<IPlayable*>> m_playbackGroups;

            // Variáveis para controlar a UI de criação de grupo
            bool m_isCreatingGroup = false;
            char m_newGroupNameBuffer[128] = "";
            std::vector<bool> m_groupCreationSelection;
    };

} // namespace Window

#endif
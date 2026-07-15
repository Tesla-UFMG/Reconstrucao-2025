#ifndef PEDAL_WINDOW_HPP
#define PEDAL_WINDOW_HPP

// C++
#include <algorithm>
#include <vector>
#include <string>

// Project
#include "AssetManager.hpp"
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"
#include "ui/windows/iWindow.hpp"
#include "DB.hpp"

// Estrutura para os dados das colunas, similar a de outras janelas
struct PedalData {
    std::string         column;
    std::string         archive;
    std::string         fileType;
    double              maxValue = 1.0;
};

class WindowManager;

namespace Window {
    class Pedal : public IWindow {
        friend class ::WindowManager;
        public:
            explicit Pedal(bool* isOpen = nullptr);
            virtual void render() override;

            bool isLoaded() const;
            double getDuration() const;

        private:
            // --- Variáveis de estado movidas para consistência ---
            bool m_isPlaying = false;
            float m_playbackSpeed = 60.0f;

            // --- Métodos privados para Drag-and-Drop ---
            void processColumnDragDrop();
            // Assinatura da função ATUALIZADA
            void addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName);
            void removeColumn(int index);

            // --- Variáveis de Estado para os Dados ---
            std::vector<PedalData> m_dataList;
            int m_throttleIndex = -1;
            int m_brakeIndex = -1;
            double m_currentTime = 0.0;
            float m_stepSize = 30.0f;

            // --- Texturas e Fontes ---
            bool m_showPedalImages = false;
    };

} // namespace Window

#endif
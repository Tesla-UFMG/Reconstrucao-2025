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
#include "ui/windows/IPlayable.hpp" // <-- ADICIONADO
#include "DB.hpp"                   // <-- ADICIONADO para acessar dados

// Estrutura para os dados das colunas, similar a de outras janelas
struct PedalData {
    std::string         column;
    std::string         archive;
    std::vector<double> data;
    double              maxValue = 1.0;
};

namespace Window {
    // A classe agora implementa IPlayable
    class Pedal : public IWindow, public IPlayable {
        public:
            explicit Pedal(bool* isOpen = nullptr);
            virtual void render() override;

            // --- Implementação da Interface IPlayable ---
            void play() override;
            void pause() override;
            void seek(double position) override;
            bool isPlaying() const override;
            bool isLoaded() const override;
            double getCurrentTime() const override;
            double getDuration() const override;
            const char* getTitle() const override;
            float getStepSize() const override;
            void setStepSize(float size) override;

        private:

            float m_playbackSpeed = 60.0f;
            bool m_isPlaying = false;

            // --- Métodos privados para Drag-and-Drop ---
            void processColumnDragDrop();
            void addColumn(const std::string& archiveName, const std::string& columnName);
            void removeColumn(int index);

            // --- Variáveis de Estado para os Dados ---
            std::vector<PedalData> m_dataList; // Lista para guardar as colunas (acelerador, freio)
            int m_throttleIndex = -1;          // Índice do acelerador na m_dataList
            int m_brakeIndex = -1;             // Índice do freio na m_dataList
            double m_currentTime = 0.0;        // Posição atual na linha do tempo
            float m_stepSize = 30.0f;          // Passo para os botões de seek

            // --- Texturas e Fontes ---
            ImTextureID redPedalTexture;
            ImTextureID greenPedalTexture;
    };

} // namespace Window

#endif
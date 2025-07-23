#ifndef WHEEL_WINDOW_HPP
#define WHEEL_WINDOW_HPP

// C++
#include <vector>
#include <string>

// Project
#include "AssetManager.hpp"
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp" 
#include "ui/windows/iWindow.hpp"
#include "ui/windows/IPlayable.hpp"
#include "DB.hpp"

#define VOLANTE_PATH "assets/volantetesla.png"

// Estrutura para os dados da coluna do volante
struct WheelData {
    std::string         column;
    std::string         archive;
    const std::vector<double>* data;
    double              maxValue = 1.0;
    double              minValue = 0.0; 
};

namespace Window {
    class WheelControl : public IWindow, public IPlayable {
        public:
            explicit WheelControl(bool* isOpen = nullptr);
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
            // --- Métodos privados para Drag-and-Drop ---
            void processColumnDragDrop();
            // Assinatura da função ATUALIZADA
            void addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName);
            void removeColumn(int index);

            // --- Variáveis de Estado para os Dados ---
            std::vector<WheelData> m_dataList;
            int m_steerIndex = -1;
            double m_currentTime = 0.0;
            bool m_isPlaying = false;
            float m_playbackSpeed = 60.0f;
            float m_stepSize = 30.0f;
            bool m_dataIsDegrees = false;
    };

} 

#endif
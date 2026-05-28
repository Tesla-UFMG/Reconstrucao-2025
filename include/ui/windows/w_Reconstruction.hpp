#ifndef CIRCUIT_RECONSTRUCTION_WINDOW_HPP
#define CIRCUIT_RECONSTRUCTION_WINDOW_HPP

// C++
#include <algorithm>
#include <cmath>
#include <cstring> // Para std::memset ou similar
#include <sstream>
#include <string>
#include <vector>

// Project
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"
#include "ui/menubar/m_Utils.hpp"
#include "ui/windows/iWindow.hpp"

#define MIN_COORD_SIZE 50.0
#define MAX_COORD_SIZE 1600.0

// As estruturas de dados permanecem fora da classe, pois são tipos de dados.
struct COORDData {
    std::string         column;
    std::string         archive;
    std::vector<double> data;
    double              multiplier;
};

struct MarkerInfo {
    size_t idx;
    ImVec2 pos;
    float  speed;
    float  acceleration;
    float  position;
    int    lap;
    int    trackIndex;
    float  trackFrac;
};

struct CommentInfo {
    size_t      idx;
    ImVec2      triOffset;
    bool        visible;
    char        text[256];
    CommentInfo() : idx(0), triOffset({0,0}), visible(false) { std::memset(text, 0, sizeof(text)); }
};

struct TrackPoint {
    float x, y;
    float referenceSpeed;
};

struct RaceData {
    bool isSaved = false;
    char name[64] = "Corrida sem nome";
    float cartHeight;
    float cartZoom;
    float speedMultiplier;
    float HighSpeedThreshold;
    float LowSpeedThreshold;
    std::vector<MarkerInfo> markedPositionsGreen;
    std::vector<MarkerInfo> markedPositionsRed;
    std::vector<std::vector<size_t>> highSpeedSegments;
    std::vector<std::vector<size_t>> lowSpeedSegments;
    std::vector<CommentInfo> comments;
    int latIndex;
    int lonIndex;
};


namespace Window {
    class Reconstruction : public IWindow {
        public:
            explicit Reconstruction(bool* isOpen = nullptr);
            virtual void render() override;

            bool isLoaded() const;


        private:
        
            static constexpr float DEFAULT_SPEED = 20.0f; // km/h

            Uint32 m_simLastTime = 0;
            bool m_showTrackInfo = false;
            bool m_showCoordinatesTable = true;
            int m_activeTab = 0;

                        // --- Funções de Renderização ---
            void RenderCoordinatesTable();
            void RenderActiveTab(int activeTab, float deltaTime);

            void RenderSimulationTab(float deltaTime);
            void RenderRaceManagementTab();
            void RenderCoordinatesDataTab();

            bool m_seekJustOccurred = false;

            // --- Funções Auxiliares de Renderização e Lógica ---
            float m_stepSize = 30.0f;
            ImU32 GetColorForSpeed(float speed);
            void  DrawTrackAndKartAt(const std::vector<ImVec2>& screenPts,
                                     const ImVec2& origin,
                                     float scale);
            void processColumnDragDrop();
             void addColumnToMap(const std::string& fileType, const std::string& fileName, const std::string& columnName);
            void removeColumnFromMap(size_t index);
            void ConvertLatLonToXY(std::vector<float>& outX, std::vector<float>& outY);
            void BuildTrackFromLatLon();
            void SalvarCorrida(int slotIndex);
            void CarregarCorrida(int slotIndex);
            void LimparCorrida(int slotIndex);

            // --- ESTADO DA JANELA (Variáveis que eram 'g_') ---
            // O estado agora é privado e pertence a cada instância da janela.
            
            // Dados da pista e coordenadas
            std::vector<TrackPoint>  m_track;
            std::vector<ImVec2>      m_screenTrack;
            std::vector<COORDData>   m_coordDataList;
            int                      m_latIndex = -1;
            int                      m_lonIndex = -1;

            // Simulação
            bool  m_isSimulating = false;
            float m_kartPosition = 0.0f;
            float m_kartSpeed = 30.0f;
            int   m_lapCount = 0;
            float m_currentAcceleration = 0.0f;
            
            // Configurações da UI de Simulação
            float m_cartHeight = 300.0f;
            float m_cartZoom = 1.0f;
            float m_speedMultiplier = 1.0f;
            float m_HighSpeedThreshold = 50.0f;
            float m_LowSpeedThreshold = 10.0f;

            // Marcadores e Comentários
            std::vector<MarkerInfo>    m_markedPositionsGreen;
            std::vector<MarkerInfo>    m_markedPositionsRed;
            std::vector<CommentInfo>   m_comments;

            // Segmentos de Velocidade
            std::vector<std::vector<size_t>> m_highSpeedSegments;
            std::vector<size_t>              m_currentHighSpeed;
            bool                             m_prevHighSpeed = false;

            std::vector<std::vector<size_t>> m_lowSpeedSegments;
            std::vector<size_t>              m_currentLowSpeed;
            bool                             m_prevLowSpeed = false;

            // Slots para salvar corridas
            static const int NUM_RACE_SLOTS = 10;
            RaceData m_savedRaces[NUM_RACE_SLOTS];
    };
} // namespace Window

#endif
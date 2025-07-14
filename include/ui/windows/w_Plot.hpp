#ifndef PLOT_WINDOW_HPP
#define PLOT_WINDOW_HPP

// Project
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/menubar/m_Utils.hpp"
#include "ui/windows/iWindow.hpp"

// C++
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#define MIN_GRAPH_SIZE  50.0
#define MAX_GRAPH_SIZE  1600.0
#define RESIZE_BAR_SIZE 3.f

enum GraphType { GRAPH_LINE, GRAPH_BAR, GRAPH_SCATTER, GRAPH_FILLED_LINE };

struct GraphData {
        std::vector<std::string> columns;   // Nome das colunas
        std::vector<std::string> fileNames; // Nome dos arquivos que vieram os dados

        std::vector<std::vector<double>>        x;          // Valores de X
        std::vector<const std::vector<double>*> y;          // Valores de Y
        std::vector<double>                     multiplier; // Multiplicador para os valores de Y

        std::string xColumn;
        GraphType   type       = GRAPH_LINE; // Tipo do gráfico
        bool        showXAxis  = false;      // Mostrar eixo X
        bool        showYAxis  = true;       // Mostrar eixo Y
        double      plotHeight = 190;        // Tamanho do gráfico
};

struct GraphData_ {
        std::string         columnName; // Nome da coluna
        std::string         fileName;   // Nome do arquivo que contém a coluna
        double              multiplier; // Multiplicador para os valores de Y
        std::vector<double> x;          // Valores de X
        std::vector<double> y;          // Valores de Y

        void buildXVector() {
            if (y.size() == x.size() || y.empty()) {
                return;
            }

            x.resize(y.size());
            for (size_t i = 0; i < y.size(); ++i) {
                x[i] = static_cast<double>(i);
            }
        }
};

struct GraphConfig {
        GraphType type       = GRAPH_LINE; // Tipo do gráfico
        bool      showXAxis  = false;      // Mostrar eixo X
        bool      showYAxis  = true;       // Mostrar eixo Y
        double    plotHeight = 190;        // Tamanho do gráfico
};

struct Graph {
        GraphConfig             config; // Configurações do gráfico
        std::vector<GraphData_> data;   // Dados do gráfico
};

namespace Window {

    class Plot : public IWindow {
        public:
            explicit Plot(bool* isOpen = nullptr);

            void render() override;

        private:
            // Funções de renderização da interface
            void drawMenuBar();

            // Funções auxiliares para processamento de dados e payload
            void processColumnDragDrop(GraphData& graphData);
            void generateSimulatedData(int numPoints, size_t graphIndex, float* x, float* y);

            void addGraph(std::vector<GraphData>& graphs);
            void removeGraph(std::vector<GraphData>& graphs, size_t graphIndex);

            void addColumnToGraph(GraphData& graphData, const ColumnPayload* payload);
            void removeColumnFromGraph(GraphData& graphData, int graphIndex);

            // Funções de renderização dos gráficos
            void drawLegendPopup(GraphData& graphData, int graphIndex);
            void renderGraph(size_t graphIndex);
            void renderResizeButton(size_t graphIndex);
    };

} // namespace Window

#endif // PLOT_WINDOW_HPP

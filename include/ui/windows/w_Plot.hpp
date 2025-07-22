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
#define RESIZE_BAR_SIZE 2.f

enum GraphType { GRAPH_LINE, GRAPH_BAR, GRAPH_SCATTER, GRAPH_FILLED_LINE };

struct GraphData {
        std::string                columnName;       // Nome da coluna
        std::string                fileName;         // Nome do arquivo que contém a coluna
        double                     multiplier = 1.0; // Multiplicador para os valores de Y
        std::vector<double>        x;                // Valores de X
        const std::vector<double>* y;                // Valores de Y

        void buildXVector() {
            const size_t newSize = y->size();
            const size_t oldSize = x.size();
            if (oldSize < newSize) {
                x.reserve(newSize);
                for (size_t i = oldSize; i < newSize; ++i) {
                    x.push_back(static_cast<double>(i));
                }
            } else if (oldSize > newSize) {
                x.resize(newSize);
            }
        };
};

struct GraphConfig {
        size_t      id;
        GraphType   type             = GRAPH_LINE; // Tipo do gráfico
        bool        showXAxis        = false;      // Mostrar eixo X
        bool        showYAxis        = true;       // Mostrar eixo Y
        int         numPoints        = 300;        // Quantidade de pontos a serem seguidos
        double      plotHeight       = 190;        // Tamanho do gráfico
        bool        followTheEnd     = false;      // Seguir o final dos dados
        bool        autoFit          = true;       // Ajustar automaticamente os eixos
        bool        showValueOnYAxis = true;       // Mostra os valores no nome da coluna
        std::string xColumn;                       // Coluna selecionada como eixo X
};

struct Graph {
        GraphConfig            config; // Configurações do gráfico
        std::vector<GraphData> data;   // Dados do gráfico

        std::vector<std::string> getColumnNames() const {
            std::vector<std::string> columnNames;
            for (const auto& graphData : data) {
                columnNames.push_back(graphData.columnName);
            }
            return columnNames;
        }
};

namespace Window {

    class Plot : public IWindow {
        public:
            explicit Plot(bool* isOpen = nullptr);
            void render() override;

        private:
            std::vector<Graph> graphs;
            bool               autoFit          = true;
            bool               showResizeButton = false;
            bool               showValueOnYAxis = true;

            // Funções de renderização da interface
            void drawMenuBar();

            // Funções auxiliares para processamento de dados e payload
            void processColumnDragDrop(Graph& graph);
            void generateSimulatedData(int numPoints, size_t graphIndex, float* x, float* y);

            void addNewGraph();
            void removeGraph(size_t graphIndex);

            void addColumnToGraph(Graph& graph, const ColumnPayload* payload);
            void removeColumnFromGraph(size_t graphIndex, size_t columnIndex);

            // Funções de renderização dos gráficos
            void drawLegendPopup(Graph& graph, size_t graphIndex);
            void renderGraph(size_t graphIndex);
            void renderResizeButton(size_t graphIndex);
    };

} // namespace Window

#endif // PLOT_WINDOW_HPP

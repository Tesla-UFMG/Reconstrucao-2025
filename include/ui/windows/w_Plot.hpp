#ifndef PLOT_WINDOW_HPP
#define PLOT_WINDOW_HPP

// Project
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
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

enum XYAlignmentMode {
    ALIGN_MIN_SIZE = 0,
    ALIGN_TIME_PROXIMITY = 1,
    ALIGN_LINEAR_INTERPOLATION = 2
};

inline std::pair<std::vector<double>, std::vector<double>> alignVectors(
    const std::vector<double>& xData,
    const std::vector<double>& yData,
    XYAlignmentMode mode
) {
    if (xData.empty() || yData.empty()) {
        return {xData, yData};
    }

    if (mode == ALIGN_MIN_SIZE) {
        size_t safeSize = std::min(xData.size(), yData.size());
        std::vector<double> xAligned(xData.begin(), xData.begin() + safeSize);
        std::vector<double> yAligned(yData.begin(), yData.begin() + safeSize);
        return {xAligned, yAligned};
    }

    size_t Nx = xData.size();
    size_t Ny = yData.size();

    if (Nx >= Ny) {
        std::vector<double> xAligned = xData;
        std::vector<double> yAligned(Nx);
        if (Nx == 1) {
            yAligned[0] = yData[0];
            return {xAligned, yAligned};
        }
        for (size_t i = 0; i < Nx; ++i) {
            double t = static_cast<double>(i) / (Nx - 1);
            double f_j = t * (Ny - 1);
            if (mode == ALIGN_TIME_PROXIMITY) {
                size_t j = static_cast<size_t>(std::round(f_j));
                if (j >= Ny) j = Ny - 1;
                yAligned[i] = yData[j];
            } else {
                size_t j_low = static_cast<size_t>(std::floor(f_j));
                size_t j_high = static_cast<size_t>(std::ceil(f_j));
                if (j_low >= Ny) j_low = Ny - 1;
                if (j_high >= Ny) j_high = Ny - 1;
                if (j_low == j_high) {
                    yAligned[i] = yData[j_low];
                } else {
                    double weight = f_j - j_low;
                    yAligned[i] = yData[j_low] * (1.0 - weight) + yData[j_high] * weight;
                }
            }
        }
        return {xAligned, yAligned};
    } else {
        std::vector<double> yAligned = yData;
        std::vector<double> xAligned(Ny);
        if (Ny == 1) {
            xAligned[0] = xData[0];
            return {xAligned, yAligned};
        }
        for (size_t j = 0; j < Ny; ++j) {
            double t = static_cast<double>(j) / (Ny - 1);
            double f_i = t * (Nx - 1);
            if (mode == ALIGN_TIME_PROXIMITY) {
                size_t i = static_cast<size_t>(std::round(f_i));
                if (i >= Nx) i = Nx - 1;
                xAligned[j] = xData[i];
            } else {
                size_t i_low = static_cast<size_t>(std::floor(f_i));
                size_t i_high = static_cast<size_t>(std::ceil(f_i));
                if (i_low >= Nx) i_low = Nx - 1;
                if (i_high >= Nx) i_high = Nx - 1;
                if (i_low == i_high) {
                    xAligned[j] = xData[i_low];
                } else {
                    double weight = f_i - i_low;
                    xAligned[j] = xData[i_low] * (1.0 - weight) + xData[i_high] * weight;
                }
            }
        }
        return {xAligned, yAligned};
    }
}

struct GraphData {
        std::string                columnName;       // Nome da coluna
        std::string                fileName;         // Nome do arquivo que contém a coluna
        std::string                fileType;         // Tipo do arquivo (CSV ou Telemetry)
        double                     multiplier = 1.0; // Multiplicador para os valores de Y
        std::vector<double>        x;                // Valores de X
        const std::vector<double>* y = nullptr;      // Valores de Y (legado, prefira getYData())

        const std::vector<double>& getYData() const {
            if (fileType == "CSV") {
                return DB::getInstance().getCSVData(fileName, columnName);
            } else if (fileType == "Telemetry") {
                return DB::getInstance().getTelemetryData(fileName, columnName);
            } else {
                return DB::getInstance().getTextData(fileName, columnName);
            }
        }

        void buildXVector() {
            const size_t newSize = getYData().size();
            const size_t oldSize = x.size();
            if (oldSize < newSize) {
                x.reserve(newSize);
                for (size_t i = oldSize; i < newSize; ++i) {
                    x.push_back(static_cast<double>(i));
                }
            } else if (oldSize > newSize) {
                x.resize(newSize);
            }
        }
};

struct GraphConfig {
        size_t      id;
        GraphType   type             = GRAPH_FILLED_LINE; // Tipo do gráfico
        bool        showXAxis        = false;      // Mostrar eixo X
        bool        showYAxis        = true;       // Mostrar eixo Y
        int         numPoints        = 500;        // Quantidade de pontos a serem seguidos
        double      plotHeight       = 250;        // Tamanho do gráfico
        bool        followTheEnd     = false;      // Seguir o final dos dados
        bool        autoFit          = true;       // Ajustar automaticamente os eixos
        bool        showValueOnYAxis = true;       // Mostra os valores no nome da coluna
        bool        showCursorOnYAxis = true;      // Mostra o cursor no eixo Y
        std::string xColumn;
        XYAlignmentMode xyAlignmentMode = ALIGN_LINEAR_INTERPOLATION;
};

struct GraphTextAnnotation {
    std::string archiveName;
    std::string columnName;
};

struct Graph {
        GraphConfig            config; // Configurações do gráfico
        std::vector<GraphData> data;   // Dados do gráfico
        std::vector<GraphTextAnnotation> textAnnotations; // Anotações textuais do gráfico

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
            std::vector<::Graph> graphs;
            bool               autoFit          = false;
            bool               showResizeButton = false;
            bool               showValueOnYAxis = true;
            bool telemetryMode = false;
            bool showCursorOnYAxis = true;

            // Funções de renderização da interface
            void drawMenuBar();

            // Funções auxiliares para processamento de dados e payload
            void processColumnDragDrop(::Graph& graph);
            void generateSimulatedData(int numPoints, size_t graphIndex, float* x, float* y);

            void addNewGraph();
            void removeGraph(size_t graphIndex);

            void addColumnToGraph(::Graph& graph, const ColumnPayload* payload);
            void removeColumnFromGraph(size_t graphIndex, size_t columnIndex);

            // Funções de renderização dos gráficos
            void drawLegendPopup(::Graph& graph, size_t graphIndex);
            void renderGraph(size_t graphIndex);
            void renderResizeButton(size_t graphIndex);
    };

} // namespace Window

#endif // PLOT_WINDOW_HPP

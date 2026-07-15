#ifndef XY_ALIGNMENT_HPP
#define XY_ALIGNMENT_HPP

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// Forward declarations or direct includes
#include "DB.hpp"

enum XYAlignmentMode { ALIGN_MIN_SIZE = 0, ALIGN_TIME_PROXIMITY = 1, ALIGN_LINEAR_INTERPOLATION = 2 };

inline std::pair<std::vector<double>, std::vector<double>>
alignVectors(const std::vector<double>& xData, const std::vector<double>& yData, XYAlignmentMode mode) {
    if (xData.empty() || yData.empty()) {
        return {xData, yData};
    }

    if (mode == ALIGN_MIN_SIZE) {
        size_t              safeSize = std::min(xData.size(), yData.size());
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
            double t   = static_cast<double>(i) / (Nx - 1);
            double f_j = t * (Ny - 1);
            if (mode == ALIGN_TIME_PROXIMITY) {
                size_t j = static_cast<size_t>(std::round(f_j));
                if (j >= Ny)
                    j = Ny - 1;
                yAligned[i] = yData[j];
            } else {
                size_t j_low  = static_cast<size_t>(std::floor(f_j));
                size_t j_high = static_cast<size_t>(std::ceil(f_j));
                if (j_low >= Ny)
                    j_low = Ny - 1;
                if (j_high >= Ny)
                    j_high = Ny - 1;
                if (j_low == j_high) {
                    yAligned[i] = yData[j_low];
                } else {
                    double weight = f_j - j_low;
                    yAligned[i]   = yData[j_low] * (1.0 - weight) + yData[j_high] * weight;
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
        for (size_t i = 0; i < Ny; ++i) {
            double t   = static_cast<double>(i) / (Ny - 1);
            double f_j = t * (Nx - 1);
            if (mode == ALIGN_TIME_PROXIMITY) {
                size_t j = static_cast<size_t>(std::round(f_j));
                if (j >= Nx)
                    j = Nx - 1;
                xAligned[i] = xData[j];
            } else {
                size_t j_low  = static_cast<size_t>(std::floor(f_j));
                size_t j_high = static_cast<size_t>(std::ceil(f_j));
                if (j_low >= Nx)
                    j_low = Nx - 1;
                if (j_high >= Nx)
                    j_high = Nx - 1;
                if (j_low == j_high) {
                    xAligned[i] = xData[j_low];
                } else {
                    double weight = f_j - j_low;
                    xAligned[i]   = xData[j_low] * (1.0 - weight) + xData[j_high] * weight;
                }
            }
        }
        return {xAligned, yAligned};
    }
}

enum GraphType { GRAPH_LINE, GRAPH_BAR, GRAPH_SCATTER, GRAPH_FILLED_LINE };

struct GraphData {
        std::string                columnName;       // Nome da coluna
        std::string                fileName;         // Nome do arquivo que contém a coluna
        std::string                fileType;         // Tipo do arquivo (CSV ou Telemetry)
        double                     multiplier = 1.0; // Multiplicador para os valores de Y
        double                     offset     = 0.0; // Soma (B) para os valores de Y
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

        // Cache para otimização do ImPlot
        std::vector<double> cachedX;
        std::vector<double> cachedY;
        size_t              lastYSize         = 0;
        size_t              lastAxisLength    = 0;
        double              lastMultiplier    = 1.0;
        double              lastOffset        = 0.0;
        bool                lastUseCustomX    = false;
        XYAlignmentMode     lastAlignmentMode = ALIGN_LINEAR_INTERPOLATION;
        std::string         lastCustomXColumn;
        size_t              lastCustomXSize = 0;

        std::vector<double> multipliedY;

        void buildMultipliedY() {
            const std::vector<double>& rawY = getYData();
            if (multiplier == 1.0 && offset == 0.0)
                return;

            size_t newSize = rawY.size();
            size_t oldSize = multipliedY.size();

            // Force full rebuild if multiplier or offset changed
            if (lastMultiplier != multiplier || lastOffset != offset) {
                multipliedY.clear();
                oldSize = 0;
            }

            if (oldSize < newSize) {
                multipliedY.reserve(newSize);
                for (size_t i = oldSize; i < newSize; ++i) {
                    multipliedY.push_back(rawY[i] * multiplier + offset);
                }
            } else if (oldSize > newSize) {
                multipliedY.resize(newSize);
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
        size_t          id;
        GraphType       type              = GRAPH_FILLED_LINE; // Tipo do gráfico
        bool            showXAxis         = false;             // Mostrar eixo X
        bool            showYAxis         = true;              // Mostrar eixo Y
        int             numPoints         = 500;               // Quantidade de pontos a ser em seguidos
        double          plotHeight        = 250;               // Tamanho do gráfico
        bool            followTheEnd      = false;             // Seguir o final dos dados
        bool            autoFit           = true;              // Ajustar automaticamente os eixos
        bool            showValueOnYAxis  = true;              // Mostra os valores no nome da coluna
        bool            showCursorOnYAxis = true;              // Mostra o cursor no eixo Y
        int             colormap          = -1;                // -1 para usar o colormap global, senao usar especifico
        std::string     xColumn;
        XYAlignmentMode xyAlignmentMode = ALIGN_LINEAR_INTERPOLATION;
};

struct GraphTextAnnotation {
        std::string archiveName;
        std::string columnName;
};

struct Graph {
        GraphConfig                      config;          // Configurações do gráfico
        std::vector<GraphData>           data;            // Dados do gráfico
        std::vector<GraphTextAnnotation> textAnnotations; // Anotações textuais do gráfico

        std::vector<std::string> getColumnNames() const {
            std::vector<std::string> columnNames;
            for (const auto& graphData : data) {
                columnNames.push_back(graphData.columnName);
            }
            return columnNames;
        }
};

#endif // XY_ALIGNMENT_HPP

#include "ui/windows/w_Telemetry.hpp"
#include "ui/windows/w_Warning.hpp"

Window::Telemetry* Window::Telemetry::s_instance = nullptr;

static std::string stripNulls(const std::string& s) {
    auto pos = s.find('\0');
    if (pos != std::string::npos)
        return s.substr(0, pos);
    return s;
}

Window::Telemetry::Telemetry(bool* isOpen) : IWindow(isOpen) {
    s_instance = this;
    title = "Telemetria";
    flags = ImGuiWindowFlags_NoScrollbar;

    // Uart configuration
    this->serialPort = "";
    this->baudrate   = 115200;
    this->getAvailablePorts();
    this->selectedPortIndex = 0;
    this->saveToFile        = false;
    this->outputPacketFolder.resize(COLUMN_NAME_SIZE);

    // Pilotos e Testes
    this->m_pilots                = {"miguel", "pedro", "boson", "RafaFreios", "Outro"};
    this->m_testTypes             = {"Aceleração", "Skidpad", "Autocross", "Endurance", "Calibração", "Outro"};
    this->m_selectedPilotIndex    = 0;
    this->m_selectedTestTypeIndex = 0;
    this->m_hasSaved              = false;
    this->m_activeCommentDates.clear();
    this->m_activeComments.clear();
    std::memset(this->m_currentCommentBuf, 0, sizeof(this->m_currentCommentBuf));

    // Packet configuration
    this->clearAndResizeInputBuffers();
    this->processingStatus     = false;
    this->m_showRecentMessages = true;
}

void Window::Telemetry::clearAndResizeInputBuffers() {
    this->packetName.clear();
    this->packetName.resize(FILE_NAME_SIZE);

    this->packetId.clear();
    this->packetId.resize(COLUMN_NAME_SIZE);

    this->packetColumnNames.clear();
    this->packetColumnNames.resize(8);
    for (int i = 0; i < 8; ++i) {
        this->packetColumnNames[i].clear();
        this->packetColumnNames[i].resize(COLUMN_NAME_SIZE);
    }
}

void Window::Telemetry::savePacketsToFile(const std::string& outputFolder) {
    DB::getInstance().saveTelemetryPackets(std::string(outputFolder.data()));
}

Window::Telemetry::~Telemetry() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
    this->closeDevice();
}

Window::Telemetry* Window::Telemetry::getInstance() {
    return s_instance;
}

void Window::Telemetry::toggleConnection() {
    int telemetryStatus = DB::getInstance().getProject().getTelemetryStatus();
    if (telemetryStatus == 1 || telemetryStatus == 2) {
        this->closeDevice();
    } else {
        if (this->serialPort.empty()) {
            this->getAvailablePorts();
            if (!this->serialPorts.empty()) {
                this->selectedPortIndex = 0;
                this->serialPort = this->serialPorts[0];
            }
        }
        if (!this->serialPort.empty()) {
            this->openDevice(this->serialPort.c_str(), this->baudrate);
        } else {
            Dialogs::showErrorDialog("Nenhuma porta serial encontrada para conexão.");
        }
    }
}

void Window::Telemetry::openDevice(const char* port, int baud) {
    closeDevice();

    if (this->device.openDevice(port, baud) == 1) {
        LOG("INFO", "Conectado na porta: " + std::string(port));
        this->keepReading  = true;
        this->readerThread = std::thread(&Telemetry::readMessages, this);
    } else {
        std::string errorMsg = "Falha ao abrir porta: " + std::string(port);
        Dialogs::showErrorDialog(errorMsg);
        LOG("ERROR", errorMsg);
    }

    DB::getInstance().getProject().setTelemetryStatus(this->device.isDeviceOpen() ? 1 : 0);
}

void Window::Telemetry::closeDevice() {
    this->keepReading = false;
    if (this->readerThread.joinable()) {
        this->readerThread.join();
    }

    if (this->device.isDeviceOpen()) {
        this->device.closeDevice();
        LOG("INFO", "Dispositivo fechado: " + this->serialPort);
    }
    DB::getInstance().getProject().setTelemetryStatus(0);
}

void Window::Telemetry::getAvailablePorts() {
    serialib    temp_device;
    std::string device_port;
    this->closeDevice();
    this->serialPorts.clear();
    for (int i = 0; i < 25; i++) {
#if defined(_WIN32) || defined(_WIN64)
        device_port = std::string("\\\\.\\COM") + std::to_string(i);
#elif defined(__linux__)
        device_port = std::string("/dev/ttyACM") + std::to_string(i - 1);
#else
#error "Unsupported operating system"
#endif

        if (temp_device.openDevice(device_port.c_str(), 115200) == 1) {
            this->serialPorts.push_back(device_port);
            LOG("INFO", "Porta encontrada: " + device_port);
            temp_device.closeDevice();
        }
    }
}

void Window::Telemetry::readMessages() {
    char buf[512];
    while (keepReading) {
        int n = device.readString(buf, '\n', sizeof(buf) - 1, 300);
        if (n > 0) {
            buf[n] = '\0';
            std::lock_guard lk(queueMutex);
            messageQueue.push(std::string(buf));
        } else if (n < 0) {
            // Conexão perdida!
            LOG("WARN", "Conexão serial perdida na porta: " + serialPort + ". Tentando reconectar...");
            DB::getInstance().getProject().setTelemetryStatus(2); // Reconectando
            device.closeDevice();

            // Loop de reconexão periódico e responsivo
            while (keepReading) {
                if (device.openDevice(serialPort.c_str(), baudrate) == 1) {
                    LOG("INFO", "Reconectado com sucesso na porta: " + serialPort);
                    DB::getInstance().getProject().setTelemetryStatus(1); // Conectado
                    break;
                }

                // Sleep de 1.5 segundos com checagem a cada 100ms
                for (int i = 0; i < 15 && keepReading; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
    }
}

void Window::Telemetry::drainQueueIntoRecent() {
    std::lock_guard lk(queueMutex);
    if (recentMessages.size() >= MAX_RECENT_MESSAGES) {
        if (this->saveToFile) {
            this->savePacketsToFile(this->outputPacketFolder);
        }
        recentMessages.clear();
    }

    while (!messageQueue.empty()) {
        recentMessages.push_back(std::move(messageQueue.front()));
        this->processingStatus = this->processPacket(recentMessages.back());
        DB::getInstance().getProject().setProcessingStatus(this->processingStatus);
        messageQueue.pop();
    }
}

void Window::Telemetry::renderConfigMenu() {

    ImGui::SeparatorText("Configuração da UART");
    ImGui::BeginGroup();
    if (ImGui::BeginTable("##cfg", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Widget", ImGuiTableColumnFlags_WidthStretch);

        // baudrate
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Baudrate");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputInt("##baudrate", &this->baudrate, 0, 0);

        // porta serial
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Porta Serial");
        ImGui::TableSetColumnIndex(1);
        {
            std::vector<const char*> items;
            items.reserve(serialPorts.size());
            for (auto& s : serialPorts)
                items.push_back(s.c_str());
            ImGui::SetNextItemWidth(150.0f);
            ImGui::Combo("##PortasSeriais", &this->selectedPortIndex, items.data(), (int)items.size());

            if (selectedPortIndex >= (int)items.size()) {
                this->serialPort = "";
            } else {
                this->serialPort = items[selectedPortIndex];
            }
        }

        // Mostrar Dados checkbox
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Mostrar Dados");
        ImGui::TableSetColumnIndex(1);
        ImGui::Checkbox("##showRecentMessages", &this->m_showRecentMessages);

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Status:");
    ImGui::SameLine();
    int         telemetryStatus = DB::getInstance().getProject().getTelemetryStatus();
    std::string status          = "Desconectado";
    ImVec4      statusColor     = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Vermelho suave
    if (telemetryStatus == 1) {
        status      = "Conectado";
        statusColor = HI(1); // Verde
    } else if (telemetryStatus == 2) {
        status      = "Reconectando...";
        statusColor = ImVec4(1.0f, 0.6f, 0.0f, 1.0f); // Laranja
    }
    ImGui::TextColored(statusColor, status.c_str());

    ImGui::SameLine();
    ImGui::Text("Processamento:");
    std::string processing = this->processingStatus ? "Ok" : "Erro";
    ImGui::SameLine();
    ImGui::TextColored(this->processingStatus ? HI(1) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f), processing.c_str());

    ImGui::Spacing();

    // Botões
    if (ImGui::Button("Conectar") && !this->serialPort.empty()) {
        this->openDevice(this->serialPort.c_str(), this->baudrate);
    }

    ImGui::SameLine();
    if (ImGui::Button("Desconectar")) {
        this->closeDevice();
    }

    ImGui::SameLine();
    if (ImGui::Button("Atualizar Portas")) {
        this->getAvailablePorts();
    }

    ImGui::SameLine();
    if (ImGui::Button("Limpar Dados")) {
        if (Dialogs::showConfirmationDialog(
                "Tem certeza que deseja limpar todos os dados recebidos dos pacotes de telemetria e comentários?")) {
            DB::getInstance().getProject().clearAllTelemetryData();
            this->m_activeCommentDates.clear();
            this->m_activeComments.clear();
            std::memset(this->m_currentCommentBuf, 0, sizeof(this->m_currentCommentBuf));
            this->m_saveCount = 0; // Reseta o contador de salvamentos

            // Limpa a tela de avisos integrada
            if (Window::Warning* warningWin = Window::Warning::getInstance()) {
                warningWin->clearLogs();
            }

            LOG("INFO", "Todos os dados telemétricos e comentários foram limpos.");
        }
    }

    ImGui::EndGroup();
}

void Window::Telemetry::renderPacketConfigMenu() {
    std::string configTitle = m_editMode ? "Configuração dos Pacotes (Modo Edição)" : "Configuração dos Pacotes";
    ImGui::SeparatorText(configTitle.c_str());
    ImGui::BeginGroup();
    if (ImGui::BeginTable("##packetCfg", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        // Nome do Pacote
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Nome do Pacote");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("##packet_name", this->packetName.data(), FILE_NAME_SIZE, ImGuiInputTextFlags_CharsNoBlank);

        // ID do Pacote
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("ID do Pacote");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("##packet_id", this->packetId.data(), COLUMN_NAME_SIZE, ImGuiInputTextFlags_CharsNoBlank);
        ImGui::EndTable();
    }

    ImGui::Separator();

    if (ImGui::BeginTable("##packetCfg", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        for (int i = 0; i < 8; ++i) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Coluna %d", i + 1);
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(150.0f);
            ImGui::InputText("##colname", this->packetColumnNames[i].data(), COLUMN_NAME_SIZE,
                             ImGuiInputTextFlags_CharsNoBlank);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    std::string buttonLabel = m_editMode ? "Salvar Alterações" : "Salvar Configuração";
    if (ImGui::Button(buttonLabel.c_str())) {
        // Verifica o nome do pacote
        std::string packetName_ = stripNulls(this->packetName);
        if (packetName_.empty()) {
            Dialogs::showErrorDialog("O nome do pacote não pode ser vazio.");
            ImGui::EndGroup();
            return;
        }

        // Verifica o id do pacote
        std::string packetId_ = stripNulls(this->packetId);
        if (packetId_.empty()) {
            Dialogs::showErrorDialog("O ID do pacote não pode ser vazio.");
            ImGui::EndGroup();
            return;
        }

        // Filtra colunas preenchidas
        std::vector<std::string> packetColumnNames_;
        for (size_t i = 0; i < this->packetColumnNames.size(); ++i) {
            std::string colName = stripNulls(this->packetColumnNames[i]);
            if (!colName.empty()) {
                packetColumnNames_.push_back(colName);
            }
        }

        // Verifica se há pelo menos 1 coluna preenchida
        if (packetColumnNames_.empty()) {
            Dialogs::showErrorDialog("Você deve preencher pelo menos 1 coluna.");
            ImGui::EndGroup();
            return;
        }

        // Verifica se os nomes das colunas preenchidas são únicos
        std::set<std::string> uniq;
        for (const auto& colName : packetColumnNames_) {
            uniq.insert(colName);
        }
        if (uniq.size() != packetColumnNames_.size()) {
            Dialogs::showErrorDialog("Os nomes das colunas não podem se repetir.");
            ImGui::EndGroup();
            return;
        }

        // Tenta salvar ou atualizar
        if (m_editMode) {
            if (DB::getInstance().getProject().updatePacket(m_editPacketId, packetId_, packetName_,
                                                            packetColumnNames_)) {
                this->clearAndResizeInputBuffers();
                m_editMode     = false;
                m_editPacketId = "";
                LOG("INFO", "Pacote atualizado: " + packetName_);
            } else {
                Dialogs::showErrorDialog(
                    "Erro ao atualizar o pacote. Verifique se o ID já está em uso por outro pacote.");
            }
        } else {
            if (DB::getInstance().getProject().loadPacket(packetName_, packetId_, packetColumnNames_)) {
                this->clearAndResizeInputBuffers();
                LOG("INFO", "Pacote salvo: " + packetName_);
            } else {
                std::string msg = "Pacote com o ID já existe. ID: " + packetId_;
                Dialogs::showErrorDialog(msg);
                LOG("ERROR", msg);
            }
        }
    }

    if (m_editMode) {
        ImGui::SameLine();
        if (ImGui::Button("Cancelar")) {
            this->clearAndResizeInputBuffers();
            m_editMode     = false;
            m_editPacketId = "";
        }
    }

    ImGui::EndGroup();

    ImGui::BeginGroup();
    ImGui::SeparatorText("Pacotes Salvos:");

    ImGuiTableFlags tblFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                               ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_NoHostExtendY |
                               ImGuiTableFlags_Resizable | ImGuiTableFlags_HighlightHoveredColumn;
    if (ImGui::BeginTable("##TelemetryTable", 11, tblFlags)) {
        // Define cabeçalhos
        ImGui::TableSetupColumn("Nome do Pacote", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthStretch);
        for (int i = 0; i < 8; ++i) {
            ImGui::TableSetupColumn(("Col. " + std::to_string(i + 1)).c_str(), ImGuiTableColumnFlags_WidthStretch);
        }
        ImGui::TableSetupColumn("Ações", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableHeadersRow();

        const auto& telemetryFiles = DB::getInstance().getProject().getTelemetryFiles();
        for (size_t i = 0; i < telemetryFiles.size(); ++i) {
            const auto& telemetryFile = telemetryFiles[i];
            ImGui::TableNextRow();

            // Coluna 0: Nome
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(telemetryFile.getName().c_str());

            // Coluna 1: ID
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(telemetryFile.getPacketId().c_str());

            // Coluna 2: Colunas
            for (int col = 0; col < 8; ++col) {
                ImGui::TableSetColumnIndex(col + 2);
                if ((size_t)col < telemetryFile.getColumnNames().size()) {
                    ImGui::TextUnformatted(telemetryFile.getColumnNames()[col].c_str());
                } else {
                    ImGui::TextUnformatted("");
                }
            }

            // Coluna 10: Botões Ações
            ImGui::TableSetColumnIndex(10);
            ImGui::PushID(telemetryFile.getPacketId().c_str());
            
            // Botão SUBIR ("^")
            if (i == 0) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("^")) {
                DB::getInstance().getProject().swapPackets(i, i - 1);
            }
            if (i == 0) {
                ImGui::EndDisabled();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Mover para cima");
            }

            ImGui::SameLine();

            // Botão DESCER ("v")
            if (i == telemetryFiles.size() - 1) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("v")) {
                DB::getInstance().getProject().swapPackets(i, i + 1);
            }
            if (i == telemetryFiles.size() - 1) {
                ImGui::EndDisabled();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Mover para baixo");
            }

            ImGui::SameLine();

            // Botão EDITAR ("E")
            if (ImGui::Button("E")) {
                m_editMode     = true;
                m_editPacketId = telemetryFile.getPacketId();

                // Carrega nome
                this->packetName.clear();
                this->packetName.resize(FILE_NAME_SIZE);
                std::strncpy(this->packetName.data(), telemetryFile.getName().c_str(), FILE_NAME_SIZE);

                // Carrega ID
                this->packetId.clear();
                this->packetId.resize(COLUMN_NAME_SIZE);
                std::strncpy(this->packetId.data(), telemetryFile.getPacketId().c_str(), COLUMN_NAME_SIZE);

                // Carrega colunas
                for (int col = 0; col < 8; ++col) {
                    this->packetColumnNames[col].clear();
                    this->packetColumnNames[col].resize(COLUMN_NAME_SIZE);
                }
                for (size_t col = 0; col < telemetryFile.getColumnNames().size() && col < 8; ++col) {
                    std::strncpy(this->packetColumnNames[col].data(), telemetryFile.getColumnNames()[col].c_str(),
                                 COLUMN_NAME_SIZE);
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Editar este pacote");
            }
            
            ImGui::SameLine();
            
            // Botão COPIAR ("C")
            if (ImGui::Button("C")) {
                m_editMode     = false;
                m_editPacketId = "";

                // Carrega nome para reaproveitar
                this->packetName.clear();
                this->packetName.resize(FILE_NAME_SIZE);
                std::strncpy(this->packetName.data(), telemetryFile.getName().c_str(), FILE_NAME_SIZE);

                // Deixa o ID em branco para o usuário digitar o novo ID único
                this->packetId.clear();
                this->packetId.resize(COLUMN_NAME_SIZE);

                // Carrega colunas para reaproveitar
                for (int col = 0; col < 8; ++col) {
                    this->packetColumnNames[col].clear();
                    this->packetColumnNames[col].resize(COLUMN_NAME_SIZE);
                }
                for (size_t col = 0; col < telemetryFile.getColumnNames().size() && col < 8; ++col) {
                    std::strncpy(this->packetColumnNames[col].data(), telemetryFile.getColumnNames()[col].c_str(),
                                 COLUMN_NAME_SIZE);
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Copiar estrutura (reaproveitar nome do arquivo e colunas)");
            }

            ImGui::SameLine();
            
            // Botão REMOVER ("X")
            if (ImGui::Button("X")) {
                if (Dialogs::showConfirmationDialog("Tem certeza que deseja remover o pacote " +
                                                    telemetryFile.getName() + "?")) {
                    if (m_editMode && m_editPacketId == telemetryFile.getPacketId()) {
                        m_editMode     = false;
                        m_editPacketId = "";
                        this->clearAndResizeInputBuffers();
                    }
                    DB::getInstance().getProject().removePacket(telemetryFile.getPacketId());
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Remover este pacote");
            }
            
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndGroup();
}

void Window::Telemetry::renderRecentMessages() {
    ImGui::SeparatorText("Mensagens Recentes");
    ImVec2 available_size  = ImGui::GetContentRegionAvail();
    available_size.y      -= ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("TelemetryScroll", available_size, true, ImGuiWindowFlags_HorizontalScrollbar);
    {
        for (const std::string& line : this->recentMessages) {
            ImGui::TextUnformatted(line.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
    if (ImGui::Button("Limpar")) {
        this->recentMessages.clear();
    }
}

// Auxiliar para formatação no cabeçalho
static std::string formatEpochToTimeLocal(const std::string& epochStr) {
    try {
        long long   ms           = std::stoll(epochStr);
        std::time_t t            = ms / 1000;
        int         milliseconds = ms % 1000;
        std::tm*    local        = std::localtime(&t);
        char        buf[64];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", local);
        std::stringstream ss;
        ss << "[" << buf << "." << std::setw(3) << std::setfill('0') << milliseconds << "]";
        return ss.str();
    } catch (...) {
        return "[" + epochStr + "]";
    }
}

void Window::Telemetry::renderSavingMenu() {
    // Sincroniza comentários do ProjectData (carregados via deserialização) com o estado da janela de telemetria
    const auto& pTextFiles = DB::getInstance().getProject().getTextFiles();
    if (!pTextFiles.empty()) {
        const auto& pDates    = pTextFiles[0].getDates();
        const auto& pComments = pTextFiles[0].getComments();
        if (m_activeComments.size() < pComments.size()) {
            m_activeCommentDates = pDates;
            m_activeComments     = pComments;
        }
    }

    ImGui::SeparatorText("Gravação e Salvamento do Projeto");
    ImGui::BeginGroup();

    if (ImGui::BeginTable("##savingCfg", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Widget", ImGuiTableColumnFlags_WidthStretch);

        // Piloto
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Piloto");
        ImGui::TableSetColumnIndex(1);
        {
            std::vector<const char*> items;
            items.reserve(m_pilots.size());
            for (auto& s : m_pilots)
                items.push_back(s.c_str());
            ImGui::SetNextItemWidth(200.0f);
            ImGui::Combo("##ComboPilotos", &this->m_selectedPilotIndex, items.data(), (int)items.size());
        }

        // Tipo de Teste
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Tipo de Teste");
        ImGui::TableSetColumnIndex(1);
        {
            std::vector<const char*> items;
            items.reserve(m_testTypes.size());
            for (auto& s : m_testTypes)
                items.push_back(s.c_str());
            ImGui::SetNextItemWidth(200.0f);
            ImGui::Combo("##ComboTestes", &this->m_selectedTestTypeIndex, items.data(), (int)items.size());
        }

        // Nome do Projeto
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Nome do Projeto");
        ImGui::TableSetColumnIndex(1);
        {
            std::string pilotName    = m_pilots[m_selectedPilotIndex];
            std::string testType     = m_testTypes[m_selectedTestTypeIndex];
            std::string baseProjName = pilotName + "_" + testType;
            ImGui::TextDisabled("%s", baseProjName.c_str());
        }

        // Salvar Automaticamente
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Salvar Automat.");
        ImGui::TableSetColumnIndex(1);
        ImGui::Checkbox("##saveInFile", &this->saveToFile);

        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Botão de Salvar
    if (ImGui::Button("Salvar")) {
        // Pega data e hora atual no formato YYYY-MM-DD_HH-MM-SS
        auto              now       = std::chrono::system_clock::now();
        auto              in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
        std::string dateTimeStr = ss.str();

        std::string pilotName = m_pilots[m_selectedPilotIndex];
        std::string testType  = m_testTypes[m_selectedTestTypeIndex];

        this->m_saveCount++; // Incrementa o número de salvamentos

        // Cria o nome do projeto adicionando a data no início e o contador no final
        std::string generatedProjName = dateTimeStr + "_" + pilotName + "_" + testType + "_V" + std::to_string(this->m_saveCount);

        // Atualiza o nome do projeto
        //DB::getInstance().getProject().currentProjectName = generatedProjName;

        // Limpa e atualiza outputPacketFolder
        this->outputPacketFolder.clear();
        this->outputPacketFolder.resize(COLUMN_NAME_SIZE);
        std::strncpy(this->outputPacketFolder.data(), generatedProjName.c_str(), COLUMN_NAME_SIZE - 1);

        // Salva os pacotes
        this->savePacketsToFile(generatedProjName);

        std::vector<std::vector<std::string>> textData(1, m_activeComments);
        DB::getInstance().getProject().addTextFile("Comentários", {"Comentários"}, m_activeCommentDates, textData);

        // Atualiza controle de tempo
        this->m_lastSaveTime = std::chrono::steady_clock::now();
        this->m_hasSaved     = true;

        LOG("INFO", "Projeto salvo como: " + generatedProjName);
    }

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    // Relação com o tempo do último salvamento
    if (this->m_hasSaved) {
        auto now     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - this->m_lastSaveTime).count();
        if (elapsed < 60) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Último salvamento: %lds atrás", elapsed);
        } else {
            long long minutes = elapsed / 60;
            long long seconds = elapsed % 60;
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Último salvamento: %lldm %llds atrás", minutes,
                               seconds);
        }
    } else {
        ImVec4 yellowColor = (ImGuiWrapper::currentTheme == LIGHT) 
                             ? ImVec4(0.55f, 0.42f, 0.0f, 1.0f) // Amarelo escuro / dourado para tema claro
                             : ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Amarelo brilhante para tema escuro
        ImGui::TextColored(yellowColor, "Último salvamento: Nunca");
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Comentários da Sessão");

    ImGui::Text("Comentário");
    ImGui::SameLine();
    bool enterPressed = ImGui::InputText("##comments_input", m_currentCommentBuf, sizeof(m_currentCommentBuf), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::Button("Adicionar") || enterPressed) {
        auto now          = std::chrono::system_clock::now();
        auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        // Subtrai o atraso especificado pelo usuário
        timestamp_ms -= static_cast<long long>(m_commentOffsetSec) * 1000;

        std::string dateStr = std::to_string(timestamp_ms);

        m_activeCommentDates.push_back(dateStr);
        m_activeComments.push_back(std::string(m_currentCommentBuf));

        // Envia imediatamente para o arquivo "Comentários" no ProjectData
        std::vector<std::vector<std::string>> textData(1, m_activeComments);
        DB::getInstance().getProject().addTextFile("Comentários", {"Comentários"}, m_activeCommentDates, textData);

        LOG("INFO", "Comentário adicionado: " + m_activeComments.back());
        std::memset(m_currentCommentBuf, 0, sizeof(m_currentCommentBuf));
    }

    ImGui::Spacing();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Atraso (s)");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputInt("##comment_offset", &m_commentOffsetSec);
    if (m_commentOffsetSec < 0)
        m_commentOffsetSec = 0;

    ImGui::Spacing();
    ImGui::Text("Comentários Registrados (%d):", (int)m_activeComments.size());
    ImVec2 childSize = ImVec2(0, 0); // Ocupa todo o espaço restante
    if (ImGui::BeginChild("##activeCommentsScroll", childSize, true)) {
        for (size_t j = 0; j < m_activeComments.size(); ++j) {
            std::string formattedTime = formatEpochToTimeLocal(m_activeCommentDates[j]);
            ImGui::TextWrapped("%s %s", formattedTime.c_str(), m_activeComments[j].c_str());
        }
    }
    ImGui::EndChild();

    ImGui::EndGroup();
}

void Window::Telemetry::render() {
    this->drainQueueIntoRecent();

    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);
        if (ImGui::BeginTabBar("TabTelemetria",
                               ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll)) {
            if (ImGui::BeginTabItem("UART")) {
                this->renderConfigMenu();
                if (this->m_showRecentMessages) {
                    this->renderRecentMessages();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Pacotes")) {
                this->renderPacketConfigMenu();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Salvamento")) {
                this->renderSavingMenu();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}

bool Window::Telemetry::processPacket(const std::string& packet) {
    // Pega a data e hora atual com milissegundos em unix time
    auto        now          = std::chrono::system_clock::now();
    auto        timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string date         = std::to_string(timestamp_ms);

    // Processa os dados
    std::vector<double> data;
    std::stringstream   ss(packet);
    std::string         item;
    std::string         packetId;
    int                 index = 0;
    while (std::getline(ss, item, ',')) {
        if (index == 0) {
            packetId = item;
        } else {
            try {
                double value = std::stod(item);
                data.push_back(value);
            } catch (const std::exception& e) {
                return false;
            }
        }
        index++;
    }

    return DB::getInstance().processTelemetryPacket(packetId, data, date);
}

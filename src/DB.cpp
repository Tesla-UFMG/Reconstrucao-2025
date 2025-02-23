#include "DB.hpp"

DB& DB::getInstance(const std::string& filepath) {
    static DB instance(filepath);
    return instance;
}

DB::DB(const std::string& filepath) {
    try {
        int SQLiteFlags = SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE;
        this->db        = std::unique_ptr<SQLite::Database>(new SQLite::Database(filepath, SQLiteFlags));
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao abrir banco de dados: ") + e.what();
        LOG("FATAL", error);
        std::cerr << error << std::endl;
        throw;
    }
    LOG("TRACE", "Banco de dados inicializado com sucesso.");
}

DB::~DB() { LOG("TRACE", "Banco de dados encerrado."); }

void DB::createTable(const std::string& table, std::vector<std::string>& columns) {
    std::string query;
    try {
        query += "CREATE TABLE " + table + " (";
        for (size_t i = 0; i < columns.size(); i++) {
            query += columns[i];
            if (i < columns.size() - 1)
                query += ", ";
        }
        query += ");";
        this->db->exec(query);
        LOG("AUDIT", "Tabela '" + table + "' criada com sucesso.");
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao criar tabela: ") + e.what();
        LOG("ERROR", error);
        LOG("DEBUG", query);
        std::cerr << error << std::endl;
        std::cerr << query << std::endl;
    }
}

void DB::dropTable(const std::string& table) {
    std::string query = "DROP TABLE " + table + ";";
    try {
        this->db->exec(query);
        LOG("AUDIT", "Tabela '" + table + "' apagada.");
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao apagar tabela: ") + e.what();
        LOG("ERROR", error);
        LOG("DEBUG", query);
        std::cerr << error << std::endl;
        std::cerr << query << std::endl;
    }
}

void DB::insert(const std::string& table, std::vector<std::string>& values) {
    std::string query;
    try {
        query = "INSERT INTO " + table + " VALUES (";

        for (size_t i = 0; i < values.size(); i++) {
            query += values[i];
            if (i < values.size() - 1)
                query += ", ";
        }
        query += ");";
        this->db->exec(query);
        LOG("AUDIT", "Inserção na tabela '" + table + "' realizada com sucesso.");
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao inserir na tabela: ") + e.what();
        LOG("ERROR", error);
        LOG("DEBUG", query);
        std::cerr << error << std::endl;
        std::cerr << query << std::endl;
    }
}

void DB::select(const std::string& table, std::vector<std::string>& columns) {
    std::string query;
    try {
        query = "SELECT ";
        for (size_t i = 0; i < columns.size(); i++) {
            query += columns[i];
            if (i < columns.size() - 1)
                query += ", ";
        }
        query += " FROM " + table + ";";
        // Passando a referencia do banco de dados, não o ponteiro
        SQLite::Statement select(*this->db, query);
        // TODO -> FAZER A FUNÇÃO PARA RETORNAR UM MAP COM OS VALORES
        while (select.executeStep()) {
            for (size_t i = 0; i < columns.size(); i++) {
                std::cout << select.getColumn(i).getString() << " ";
            }
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao selecionar da tabela: ") + e.what();
        LOG("ERROR", error);
        LOG("DEBUG", query);
        std::cerr << error << std::endl;
        std::cerr << query << std::endl;
    }
}

void DB::update(const std::string& table, std::vector<std::string>& columns, std::vector<std::string>& values) {
    std::string query;
    try {
        query = "UPDATE " + table + " SET ";
        for (size_t i = 0; i < columns.size(); i++) {
            query += columns[i] + " = " + values[i];
            if (i < columns.size() - 1)
                query += ", ";
        }
        query += ";";
        this->db->exec(query);
        LOG("AUDIT", "Atualização na tabela '" + table + "' realizada com sucesso.");
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao atualizar na tabela: ") + e.what();
        LOG("ERROR", error);
        LOG("DEBUG", query);
        std::cerr << error << std::endl;
        std::cerr << query << std::endl;
    }
}

void DB::remove(const std::string& table, std::vector<std::string>& columns, std::vector<std::string>& condition,
                std::vector<std::string>& values) {
    std::string query;
    try {
        query = "DELETE FROM " + table + " WHERE ";
        for (size_t i = 0; i < columns.size(); i++) {
            query += columns[i] + condition[i] + values[i];
            if (i < columns.size() - 1)
                query += " AND ";
        }
        query += ";";
        this->db->exec(query);
        LOG("AUDIT", "Remoção na tabela '" + table + "' realizada com sucesso.");
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao remover da tabela: ") + e.what();
        LOG("ERROR", error);
        LOG("DEBUG", query);
        std::cerr << error << std::endl;
        std::cerr << query << std::endl;
    }
}

void DB::execute(const std::string& query) {
    try {
        this->db->exec(query);
        LOG("AUDIT", "Query executada com sucesso.");
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao executar query: ") + e.what();
        LOG("ERROR", error);
        LOG("DEBUG", query);
        std::cerr << error << std::endl;
        std::cerr << query << std::endl;
    }
}

void DB_TEST() {
    DB& db = DB::getInstance("test.db");

    // Criando tabela
    std::vector<std::string> columns = {"id INTEGER PRIMARY KEY", "nome TEXT"};
    db.createTable("teste", columns);

    // Inserindo dados
    std::vector<std::string> values1 = {"1", "'Alice'"};
    std::vector<std::string> values2 = {"2", "'Bob'"};
    db.insert("teste", values1);
    db.insert("teste", values2);

    // Selecionando dados
    std::vector<std::string> selectColumns = {"id", "nome"};
    db.select("teste", selectColumns);

    // Atualizando um valor
    std::vector<std::string> updateColumns = {"nome"};
    std::vector<std::string> updateValues  = {"'Charlie'"};
    db.update("teste", updateColumns, updateValues);

    // Selecionando dados após update
    db.select("teste", selectColumns);

    // Removendo um registro
    std::vector<std::string> removeColumns    = {"id"};
    std::vector<std::string> removeConditions = {"="};
    std::vector<std::string> removeValues     = {"1"};
    db.remove("teste", removeColumns, removeConditions, removeValues);

    // Selecionando dados após remoção
    db.select("teste", selectColumns);

    // Deletando tabela
    //    db.dropTable("teste");
}

#include "DB.hpp"

DB& DB::getInstance(const std::string& filepath) {
    static DB instance(filepath);
    return instance;
}

DB::DB(const std::string& filepath) {
    try{
        int SQLiteFlags = SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE;
        this->db = std::unique_ptr<SQLite::Database>(new SQLite::Database(filepath, SQLiteFlags));
    } catch (const std::exception&e) {
        std::string error = std::string("Erro ao abrir banco de dados: ") + e.what();
        Log::getInstance().message("FATAL", error);
        std::cerr << error << std::endl;
        throw;
    }
    Log::getInstance().message("TRACE", "Banco de dados inicializado com sucesso.");
}

DB::~DB() { 
    Log::getInstance().message("TRACE", "Banco de dados encerrado.");
}

void DB::dropTable(const std::string& table) {
    try{
        this->db->exec("DROP TABLE " + table);
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao apagar tabela: ") + e.what();
        Log::getInstance().message("ERROR", error);
        std::cerr << error << std::endl;   
    }
}

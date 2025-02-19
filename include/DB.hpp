#ifndef DB_HPP
#define DB_HPP

// Third lib
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

// C++
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Project
#include "Log.hpp"

#define DB_PATH "data.db3"

class DB {
    private:
        explicit DB(const std::string& filepath = DB_PATH);
        std::unique_ptr<SQLite::Database> db;

    public:
        DB(DB&&)            = delete;
        DB& operator=(DB&&) = delete;
        ~DB();

        static DB& getInstance(const std::string& filepath = DB_PATH);

        std::vector<const std::string&> getTables();
        void                            createTable(const std::string& table, std::vector<std::string>& columns);
        void                            dropTable(const std::string& table);
        void                            insert(const std::string& table, std::vector<std::string>& values);
        void                            select(const std::string& table, std::vector<std::string>& columns);
        void update(const std::string& table, std::vector<std::string>& columns, std::vector<std::string>& values);
        void remove(const std::string& table, std::vector<std::string>& columns, std::vector<std::string>& condition,
                    std::vector<std::string>& values);
        void execute(const std::string& query);
};

void DB_TEST();

#endif // DB_HPP
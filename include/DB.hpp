#ifndef DB_HPP
#define DB_HPP

// Third lib
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

// C++
#include <iostream>
#include <string>
#include <memory>
#include <vector>

// Project
#include "Log.hpp"

#define DB_PATH "data.db3"

class DB {
    private:
    explicit DB(const std::string& filepath = DB_PATH);
    std::unique_ptr<SQLite::Database> db;

    public:
    DB(DB&&) = delete;
    DB& operator= (DB&&) = delete;
    ~DB();

    static DB& getInstance(const std::string& filepath = DB_PATH);

    std::vector<const std::string&> getTables();
    void createTable(const std::string& name, std::vector<const std::string&> columns);
    void insert(const std::string& table, std::vector<const std::string&> columns, std::vector<const std::string&> values);
    void select(const std::string& table, std::vector<const std::string&> columns);
    void update(const std::string& table, std::vector<const std::string&> columns, std::vector<const std::string&> values);
    void remove(const std::string& table, std::vector<const std::string&> columns, std::vector<const std::string&> values);
    void dropTable(const std::string& table);
    void dropDatabase();
    void execute(const std::string& query);

};

#endif // DB_HPP
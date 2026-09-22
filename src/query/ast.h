struct Expr {
    std::string column;
    Value literal;
};

struct SelectStmt {
    std::string table;
    std::optional<Expr> where;
};

struct InsertStmt {
    std::string table;
    std::vector<Value> values;
};

struct DeleteStmt {
    std::string table;
    std::optional<Expr> where;
};

struct CreateTableStmt {
    std::string table;
    std::vector<Column> columns;
};

using Statement = std::variant<SelectStmt, InsertStmt, DeleteStmt, CreateTableStmt>;

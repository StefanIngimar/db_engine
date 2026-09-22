class Executor {
public:
    explicit Executor(Catalog& catalog);
    std::vector<Row> Execute(const Statement& stmt);
private:
    std::vector<Row> ExecSelect(const SelectStmt&);
    void ExecInsert(const InsertStmt&);
    void ExecDelete(const DeleteStmt&);
    void ExecCreateTable(const CreateTableStmt&);
    Catalog& catalog_;
};

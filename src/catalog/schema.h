struct Column{
    std::string name;
    ValueType type;
    size_t offset;
    size_t size;
};

struct Schema{
    std::string table_name;
    std::vector<Column> columns;
    size_t row_size;
};

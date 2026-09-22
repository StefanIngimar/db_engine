#include <unordered_map>
class Catalog{
    public:
        bool createTable(const std::string& name, std::vector<Column> cols);
        Schema* getSchema(const std::string& name);
        BPlusTree* getIndex(const std::string& name);

    private:
        std::unordered_map<std::string, Schema> schemas_;
        std::unordered_map<std::string, std::unique_ptr<BPlusTree>> indexes_;
};

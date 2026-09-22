enum class ValueType{INTEGER, VARCHAR};

struct Value{
    ValueType type;
    int32_t int_val = 0;
    std::string str_val;
};

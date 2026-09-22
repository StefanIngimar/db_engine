class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    Statement ParseStatement();
private:
    Statement ParseSelect();
    Statement ParseInsert();
    Statement ParseDelete();
    Statement ParseCreateTable();
    Expr ParseWhereClause();
    Token Expect(TokenType type);
    Token Peek() const;
    Token Advance();

    std::vector<Token> tokens_;
    size_t pos_ = 0;
};

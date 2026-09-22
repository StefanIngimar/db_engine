enum class TokenType{
    SELECT, INSERT, DELETE, CREATE,
    TABLE, INTO, VALUES, WHERE, FROM,
    IDENTIFIER, INT_LITERAL, STRING_LITERAL,
    EQUALS, STAR, COMMA, LPAREN, RPAREN,
    SEMICOLON, END_OF_INPUT, INVALID
};

struct Token{
    TokenType type;
    std::string text;
};

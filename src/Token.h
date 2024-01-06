#ifndef MINIC_TOKEN_H
#define MINIC_TOKEN_H

#define TOKEN_MAX_IDENTIFIER_LENGTH 64

enum TokenType {
    // Others
    TOKEN_END_OF_FILE,
    TOKEN_IDENTIFIER,

    // Literals
    TOKEN_LITERAL_NUMBER,

    // Signs
    TOKEN_COMMA,                    // ,
    TOKEN_SEMICOLON,                // ;
    TOKEN_LEFT_ROUND_BRACKET,       // (
    TOKEN_RIGHT_ROUND_BRACKET,      // )
    TOKEN_LEFT_CURLY_BRACKET,       // {
    TOKEN_RIGHT_CURLY_BRACKET,      // }

    TOKEN_EQUALS,                   // =
    TOKEN_2_EQUALS,                 // ==

    TOKEN_STAR,                     // *
    TOKEN_EXCLAMATION_MARK_EQUALS,  // !=
    TOKEN_SLASH,                    // /
    TOKEN_PERCENTAGE,               // %
    TOKEN_PLUS,                     // +
    TOKEN_AMPERSAND,                // &
    TOKEN_MINUS,                    // -

    TOKEN_LESS_THAN,                // <
    TOKEN_LESS_THAN_EQUALS,         // <=

    TOKEN_GREATER_THAN,             // >
    TOKEN_GREATER_THAN_EQUALS,      // >=


    // Keywords
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_WHILE,

    TOKEN_COUNT,
};

struct Token {
    int line;
    char *location;
    enum TokenType type;
    union {
        int int_value;
        char str_value[TOKEN_MAX_IDENTIFIER_LENGTH];
    };
};

#endif // MINIC_TOKEN_H

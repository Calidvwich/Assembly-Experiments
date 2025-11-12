#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include <set>
#include <algorithm>

using namespace std;

enum TokenType {
    TOK_EOF,
    TOK_INT, TOK_VOID, TOK_IF, TOK_ELSE, TOK_WHILE,
    TOK_BREAK, TOK_CONTINUE, TOK_RETURN,
    TOK_ID, TOK_NUMBER,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_DIV, TOK_MOD,
    TOK_LT, TOK_LE, TOK_GT, TOK_GE, TOK_EQ, TOK_NE,
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_ASSIGN,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_SEMICOLON, TOK_COMMA
};

struct Token {
    TokenType type;
    string value;
    int line;
};

class Lexer {
private:
    string input;
    size_t pos;
    int line;
    map<int, string> errors;

    char peek(int offset = 0) {
        if (pos + offset >= input.length()) return '\0';
        return input[pos + offset];
    }

    char advance() {
        if (pos >= input.length()) return '\0';
        char ch = input[pos++];
        if (ch == '\n') line++;
        return ch;
    }

    void skipWhitespace() {
        while (isspace(peek())) {
            advance();
        }
    }

    bool skipComment() {
        if (peek() == '/' && peek(1) == '/') {
            while (peek() != '\n' && peek() != '\0') advance();
            return true;
        }
        if (peek() == '/' && peek(1) == '*') {
            int startLine = line;
            advance(); advance();
            while (true) {
                if (peek() == '\0') {
                    errors[startLine] = "Unterminated comment";
                    return false;
                }
                if (peek() == '*' && peek(1) == '/') {
                    advance(); advance();
                    break;
                }
                advance();
            }
            return true;
        }
        return false;
    }

public:
    Lexer(const string& src) : input(src), pos(0), line(1) {}

    map<int, string> getErrors() { return errors; }

    Token nextToken() {
        while (true) {
            skipWhitespace();
            if (!skipComment()) break;
        }

        Token tok;
        tok.line = line;

        if (peek() == '\0') {
            tok.type = TOK_EOF;
            return tok;
        }

        if (isalpha(peek()) || peek() == '_') {
            string id;
            while (isalnum(peek()) || peek() == '_') {
                id += advance();
            }
            tok.value = id;

            if (id == "int") tok.type = TOK_INT;
            else if (id == "void") tok.type = TOK_VOID;
            else if (id == "if") tok.type = TOK_IF;
            else if (id == "else") tok.type = TOK_ELSE;
            else if (id == "while") tok.type = TOK_WHILE;
            else if (id == "break") tok.type = TOK_BREAK;
            else if (id == "continue") tok.type = TOK_CONTINUE;
            else if (id == "return") tok.type = TOK_RETURN;
            else tok.type = TOK_ID;

            return tok;
        }

        if (isdigit(peek())) {
            string num;
            while (isdigit(peek())) {
                num += advance();
            }
            tok.type = TOK_NUMBER;
            tok.value = num;
            return tok;
        }

        char ch = peek();
        switch (ch) {
            case '+': advance(); tok.type = TOK_PLUS; return tok;
            case '-': advance(); tok.type = TOK_MINUS; return tok;
            case '*': advance(); tok.type = TOK_STAR; return tok;
            case '/': advance(); tok.type = TOK_DIV; return tok;
            case '%': advance(); tok.type = TOK_MOD; return tok;
            case '(': advance(); tok.type = TOK_LPAREN; return tok;
            case ')': advance(); tok.type = TOK_RPAREN; return tok;
            case '{': advance(); tok.type = TOK_LBRACE; return tok;
            case '}': advance(); tok.type = TOK_RBRACE; return tok;
            case ';': advance(); tok.type = TOK_SEMICOLON; return tok;
            case ',': advance(); tok.type = TOK_COMMA; return tok;
            case '<':
                advance();
                if (peek() == '=') {
                    advance();
                    tok.type = TOK_LE;
                } else {
                    tok.type = TOK_LT;
                }
                return tok;
            case '>':
                advance();
                if (peek() == '=') {
                    advance();
                    tok.type = TOK_GE;
                } else {
                    tok.type = TOK_GT;
                }
                return tok;
            case '=':
                advance();
                if (peek() == '=') {
                    advance();
                    tok.type = TOK_EQ;
                } else {
                    tok.type = TOK_ASSIGN;
                }
                return tok;
            case '!':
                advance();
                if (peek() == '=') {
                    advance();
                    tok.type = TOK_NE;
                } else {
                    tok.type = TOK_NOT;
                }
                return tok;
            case '&':
                advance();
                if (peek() == '&') {
                    advance();
                    tok.type = TOK_AND;
                    return tok;
                }
                break;
            case '|':
                advance();
                if (peek() == '|') {
                    advance();
                    tok.type = TOK_OR;
                    return tok;
                }
                break;
        }
        
        advance();
        tok.type = TOK_EOF;
        return tok;
    }
};

class Parser {
private:
    vector<Token> tokens;
    size_t pos;
    map<int, string> errors;
    int loopDepth;
    bool hasError;

    Token current() {
        if (pos >= tokens.size()) return tokens.back();
        return tokens[pos];
    }

    Token peek(int offset = 0) {
        if (pos + offset >= tokens.size()) return tokens.back();
        return tokens[pos + offset];
    }

    void advance() {
        if (pos < tokens.size()) pos++;
    }

    void error(const string& msg) {
        hasError = true;
        int line = current().line;
        if (errors.find(line) == errors.end()) {
            errors[line] = msg;
        }
    }

    bool match(TokenType type) {
        return current().type == type;
    }

    bool consume(TokenType type, const string& errMsg) {
        if (match(type)) {
            advance();
            return true;
        }
        error(errMsg);
        return false;
    }

    void sync() {
        while (!match(TOK_EOF) && !match(TOK_SEMICOLON) && !match(TOK_RBRACE)) {
            advance();
        }
        if (match(TOK_SEMICOLON)) advance();
    }

    void parseCompUnit() {
        while (!match(TOK_EOF)) {
            parseFuncDef();
        }
    }

    void parseFuncDef() {
        if (!match(TOK_INT) && !match(TOK_VOID)) {
            error("Expected function return type");
            sync();
            if (match(TOK_RBRACE)) advance();
            return;
        }
        advance();

        if (!consume(TOK_ID, "Expected function name")) {
            sync();
            if (match(TOK_RBRACE)) advance();
            return;
        }

        consume(TOK_LPAREN, "Lack of '('");

        if (match(TOK_INT)) {
            parseParam();
            while (match(TOK_COMMA)) {
                advance();
                parseParam();
            }
        }

        consume(TOK_RPAREN, "Lack of ')'");
        parseBlock();
    }

    void parseParam() {
        consume(TOK_INT, "Expected int");
        consume(TOK_ID, "Expected identifier");
    }

    void parseBlock() {
        if (!consume(TOK_LBRACE, "Lack of '{'")) {
            return;
        }

        while (!match(TOK_RBRACE) && !match(TOK_EOF)) {
            parseStmt();
        }

        consume(TOK_RBRACE, "Lack of '}'");
    }

    void parseStmt() {
        if (match(TOK_INT)) {
            advance();
            consume(TOK_ID, "Expected identifier");
            if (match(TOK_ASSIGN)) {
                advance();
                parseExpr();
            }
            // 循环解析后续用逗号分隔的变量
            while (match(TOK_COMMA)) {
                advance(); // 消耗逗号
                consume(TOK_ID, "Expected identifier");
                if (match(TOK_ASSIGN)) {
                    advance();
                    parseExpr();
                }
            }
            consume(TOK_SEMICOLON, "Lack of ';'");
        } else if (match(TOK_IF)) {
            advance();
            consume(TOK_LPAREN, "Lack of '('");
            parseExpr();
            consume(TOK_RPAREN, "Lack of ')'");
            parseStmt();
            if (match(TOK_ELSE)) {
                advance();
                parseStmt();
            }
        } else if (match(TOK_WHILE)) {
            advance();
            consume(TOK_LPAREN, "Lack of '('");
            parseExpr();
            consume(TOK_RPAREN, "Lack of ')'");
            loopDepth++;
            parseStmt();
            loopDepth--;
        } else if (match(TOK_BREAK)) {
            advance();
            consume(TOK_SEMICOLON, "Lack of ';'");
        } else if (match(TOK_CONTINUE)) {
            advance();
            consume(TOK_SEMICOLON, "Lack of ';'");
        } else if (match(TOK_RETURN)) {
            advance();
            if (!match(TOK_SEMICOLON)) {
                parseExpr();
            }
            consume(TOK_SEMICOLON, "Lack of ';'");
        } else if (match(TOK_LBRACE)) {
            parseBlock();
        } else if (match(TOK_ID)) {
            advance();
            if (match(TOK_ASSIGN)) {
                advance();
                parseExpr();
                consume(TOK_SEMICOLON, "Lack of ';'");
            } else if (match(TOK_LPAREN)) {
                advance();
                if (!match(TOK_RPAREN)) {
                    parseExpr();
                    while (match(TOK_COMMA)) {
                        advance();
                        parseExpr();
                    }
                }
                consume(TOK_RPAREN, "Lack of ')'");
                consume(TOK_SEMICOLON, "Lack of ';'");
            } else {
                consume(TOK_SEMICOLON, "Lack of ';'");
            }
        } else if (match(TOK_SEMICOLON)) {
            advance();
        } else {
            error("Unexpected token");
            advance();
        }
    }

    void parseExpr() {
        parseLOrExpr();
    }

    // LOrExpr → LAndExpr ("||" LAndExpr)*
    void parseLOrExpr() {
        parseLAndExpr();
        while (match(TOK_OR)) {
            advance();
            parseLAndExpr();
        }
    }

    // LAndExpr → RelExpr ("&&" RelExpr)*
    void parseLAndExpr() {
        parseRelExpr();
        while (match(TOK_AND)) {
            advance();
            parseRelExpr();
        }
    }

    // RelExpr → AddExpr (("<" | ">" | ...) AddExpr)*
    void parseRelExpr() {
        parseAddExpr();
        while (match(TOK_LT) || match(TOK_LE) || match(TOK_GT) || 
               match(TOK_GE) || match(TOK_EQ) || match(TOK_NE)) {
            advance();
            parseAddExpr();
        }
    }

    // AddExpr → MulExpr (("+" | "-") MulExpr)*
    void parseAddExpr() {
        parseMulExpr();
        while (match(TOK_PLUS) || match(TOK_MINUS)) {
            advance();
            parseMulExpr();
        }
    }

    // MulExpr → UnaryExpr (("*" | "/" | "%") UnaryExpr)*
    void parseMulExpr() {
        parseUnaryExpr();
        while (match(TOK_STAR) || match(TOK_DIV) || match(TOK_MOD)) {
            advance();
            parseUnaryExpr();
        }
    }

    void parseUnaryExpr() {
        if (match(TOK_PLUS) || match(TOK_MINUS) || match(TOK_NOT)) {
            advance();
            parseUnaryExpr();
        } else {
            parsePrimaryExpr();
        }
    }

    void parsePrimaryExpr() {
        if (match(TOK_ID)) {
            advance();
            if (match(TOK_LPAREN)) {
                advance();
                if (!match(TOK_RPAREN)) {
                    parseExpr();
                    while (match(TOK_COMMA)) {
                        advance();
                        parseExpr();
                    }
                }
                consume(TOK_RPAREN, "Lack of ')'");
            }
        } else if (match(TOK_NUMBER)) {
            advance();
        } else if (match(TOK_LPAREN)) {
            advance();
            parseExpr();
            consume(TOK_RPAREN, "Lack of ')'");
        } else {
            error("Expected expression");
            if (!match(TOK_EOF) && !match(TOK_SEMICOLON)) {
                advance();
            }
        }
    }

public:
    Parser(const vector<Token>& toks) : tokens(toks), pos(0), loopDepth(0), hasError(false) {}

    bool parse() {
        parseCompUnit();
        return !hasError;
    }

    map<int, string> getErrors() { return errors; }
};

int main() {
    string input, line;
    while (getline(cin, line)) {
        input += line + "\n";
    }

    Lexer lexer(input);
    vector<Token> tokens;
    
    while (true) {
        Token tok = lexer.nextToken();
        tokens.push_back(tok);
        if (tok.type == TOK_EOF) break;
    }

    auto lexErrors = lexer.getErrors();

    Parser parser(tokens);
    bool success = parser.parse();
    auto parseErrors = parser.getErrors();

    map<int, string> allErrors = lexErrors;
    for (const auto& e : parseErrors) {
        allErrors[e.first] = e.second;
    }

    if (allErrors.empty()) {
        cout << "accept" << endl;
    } else {
        cout << "reject" << endl;
        for (const auto& e : allErrors) {
            cout << e.first << " " << e.second << endl;
        }
    }

    return 0;
}
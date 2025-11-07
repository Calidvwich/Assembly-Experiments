// toycc.cpp
// Single-file ToyC parser: lexer + recursive-descent parser + basic semantic checks
// Output: "accept" or "reject" then error line numbers (1-based) as required.
//
// Compile: g++ -std=c++17 toycc.cpp -O2 -o toycc

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <queue>
using namespace std;


// --------------------------- Token & Lexer --------------------------------

enum class TokType {
    END,
    ID,
    NUMBER,
    // punctuators / operators
    PLUS, MINUS, MUL, DIV, MOD,
    ASSIGN, SEMI, COMMA,
    LPAREN, RPAREN, LBRACE, RBRACE,
    LT, GT, LE, GE, EQ, NE,
    LAND, LOR, LNOT,
    KW_INT, KW_VOID, KW_IF, KW_ELSE, KW_WHILE, KW_BREAK, KW_CONTINUE, KW_RETURN,
    // unknown / error token
    UNKNOWN
};

struct Token {
    TokType type;
    string lexeme;
    int line;
};

string to_string_tok(TokType t){
    switch(t){
        case TokType::END: return "END";
        case TokType::ID: return "ID";
        case TokType::NUMBER: return "NUMBER";
        default: return "TOK";
    }
}

class Lexer {
    istream &in;
    int line = 1;
    int cur = EOF;
public:
    Lexer(istream &is): in(is) { cur = in.get(); }
    int peek() { return cur; }
    void consume() { 
        if(cur == '\n') ++line;
        cur = in.get(); 
    }

    bool isIdentStart(int c){ return c == '_' || isalpha(c); }
    bool isIdentPart(int c){ return c == '_' || isalnum(c); }

    Token nextToken(){
        while(cur != EOF && isspace(cur)) { consume(); }
        // skip comments
        if(cur == '/'){
            int next = in.peek();
            if(next == '/'){
                // single-line
                consume(); // '/'
                consume(); // second '/'
                while(cur != EOF && cur != '\n') consume();
                return nextToken();
            } else if(next == '*'){
                // multi-line comment
                consume(); // '/'
                consume(); // '*'
                bool closed = false;
                while(cur != EOF){
                    if(cur == '*'){
                        consume();
                        if(cur == '/'){
                            consume();
                            closed = true;
                            break;
                        }
                    } else {
                        consume();
                    }
                }
                if(!closed){
                    // unterminated comment: produce token to indicate error
                    Token t; t.type = TokType::UNKNOWN; t.lexeme = "Unterminated comment"; t.line = line;
                    return t;
                }
                return nextToken();
            }
        }

        Token tok;
        tok.line = line;
        if(cur == EOF || cur == -1){
            tok.type = TokType::END; tok.lexeme = ""; return tok;
        }

        // identifiers / keywords
        if(isIdentStart(cur)){
            string s;
            while(cur != EOF && isIdentPart(cur)){
                s.push_back((char)cur);
                consume();
            }
            tok.lexeme = s;
            if(s == "int") tok.type = TokType::KW_INT;
            else if(s == "void") tok.type = TokType::KW_VOID;
            else if(s == "if") tok.type = TokType::KW_IF;
            else if(s == "else") tok.type = TokType::KW_ELSE;
            else if(s == "while") tok.type = TokType::KW_WHILE;
            else if(s == "break") tok.type = TokType::KW_BREAK;
            else if(s == "continue") tok.type = TokType::KW_CONTINUE;
            else if(s == "return") tok.type = TokType::KW_RETURN;
            else tok.type = TokType::ID;
            return tok;
        }

        // number (decimal, optional leading '-': note lexer spec allows -? in regex but
        // here we treat '-' as operator; parser will accept unary minus)
        if(isdigit(cur)){
            string s;
            while(cur != EOF && isdigit(cur)){
                s.push_back((char)cur);
                consume();
            }
            tok.type = TokType::NUMBER;
            tok.lexeme = s;
            return tok;
        }

        // operators and punctuators
        switch(cur){
            case '+': tok.type = TokType::PLUS; tok.lexeme = "+"; consume(); return tok;
            case '-': tok.type = TokType::MINUS; tok.lexeme = "-"; consume(); return tok;
            case '*': tok.type = TokType::MUL; tok.lexeme = "*"; consume(); return tok;
            case '/': tok.type = TokType::DIV; tok.lexeme = "/"; consume(); return tok;
            case '%': tok.type = TokType::MOD; tok.lexeme = "%"; consume(); return tok;
            case ';': tok.type = TokType::SEMI; tok.lexeme = ";"; consume(); return tok;
            case ',': tok.type = TokType::COMMA; tok.lexeme = ","; consume(); return tok;
            case '(': tok.type = TokType::LPAREN; tok.lexeme = "("; consume(); return tok;
            case ')': tok.type = TokType::RPAREN; tok.lexeme = ")"; consume(); return tok;
            case '{': tok.type = TokType::LBRACE; tok.lexeme = "{"; consume(); return tok;
            case '}': tok.type = TokType::RBRACE; tok.lexeme = "}"; consume(); return tok;
            case '!': {
                consume();
                if(cur == '='){ consume(); tok.type = TokType::NE; tok.lexeme = "!="; return tok; }
                tok.type = TokType::LNOT; tok.lexeme = "!"; return tok;
            }
            case '=': {
                consume();
                if(cur == '='){ consume(); tok.type = TokType::EQ; tok.lexeme = "=="; return tok; }
                tok.type = TokType::ASSIGN; tok.lexeme = "="; return tok;
            }
            case '<': {
                consume();
                if(cur == '='){ consume(); tok.type = TokType::LE; tok.lexeme = "<="; return tok; }
                tok.type = TokType::LT; tok.lexeme = "<"; return tok;
            }
            case '>': {
                consume();
                if(cur == '='){ consume(); tok.type = TokType::GE; tok.lexeme = ">="; return tok; }
                tok.type = TokType::GT; tok.lexeme = ">"; return tok;
            }
            case '&': {
                consume();
                if(cur == '&'){ consume(); tok.type = TokType::LAND; tok.lexeme = "&&"; return tok; }
                // single & not in grammar -> unknown
                tok.type = TokType::UNKNOWN; tok.lexeme = "&"; return tok;
            }
            case '|': {
                consume();
                if(cur == '|'){ consume(); tok.type = TokType::LOR; tok.lexeme = "||"; return tok; }
                tok.type = TokType::UNKNOWN; tok.lexeme = "|"; return tok;
            }
            default:
                // unknown char
                {
                    char c = (char)cur;
                    tok.type = TokType::UNKNOWN;
                    tok.lexeme = string(1, c);
                    consume();
                    return tok;
                }
        }
    }
};

// --------------------------- Parser / Semantic ----------------------------

class Parser {
    Lexer lex;
    Token cur;
    vector<int> error_lines; // unique ordered
    set<int> error_set;
public:
    Parser(istream &is): lex(is) { cur = lex.nextToken(); }

    void add_error(int ln){
        if(!error_set.count(ln)){
            error_lines.push_back(ln);
            error_set.insert(ln);
        } else {
            // still preserve order; duplicates ignored
        }
    }

    void advance(){ cur = lex.nextToken(); }

    bool accept(TokType t){
        if(cur.type == t){ advance(); return true; }
        return false;
    }

    bool expect(TokType t){
        if(cur.type == t){ advance(); return true; }
        add_error(cur.line);
        // attempt recovery: don't consume (so caller can sync) — but to avoid infinite loop, advance once
        // better strategy: leave token and let sync functions skip to next safe point
        return false;
    }

    // synchronization: skip tokens until one matches any of sync set
    void sync_until(const set<TokType>& syncset){
        int iter = 0;
        while(cur.type != TokType::END && !syncset.count(cur.type) && iter < 100000){
            advance();
            ++iter;
        }
    }

    // symbol tables for functions and variables (very simple)
    struct FuncInfo { bool is_int; vector<string> params; int decl_line; };
    map<string, FuncInfo> functions;
    struct VarInfo { bool declared; int decl_line; };
    vector<map<string, VarInfo>> var_scopes; // stack of scopes

    // context flags
    bool in_loop = false;
    bool has_main = false;
    bool parse_failed = false;

    // ---------- parsing entry ----------
    void parseCompUnit(){
        // CompUnit -> FuncDef+
        while(cur.type != TokType::END){
            if(cur.type == TokType::KW_INT || cur.type == TokType::KW_VOID){
                parseFuncDef();
            } else {
                // unexpected token at top-level: try to recover by skipping to next possible function start
                add_error(cur.line);
                // skip until next 'int' or 'void' or EOF
                while(cur.type != TokType::END && cur.type != TokType::KW_INT && cur.type != TokType::KW_VOID){
                    advance();
                }
            }
        }
        // semantic: must have int main() with no params
        auto it = functions.find("main");
        if(it == functions.end() || !it->second.is_int || !it->second.params.empty()){
            // can't find proper main; choose appropriate line: if main declared, use its decl line else line 1?
            int l = (it != functions.end()) ? it->second.decl_line : 1;
            add_error(l);
        }
    }

    // FuncDef → (“int” | “void”) ID “(” (Param (“,” Param)*)? “)” Block
    void parseFuncDef(){
        bool is_int = false;
        if(cur.type == TokType::KW_INT) { is_int = true; advance(); }
        else if(cur.type == TokType::KW_VOID) { is_int = false; advance(); }
        else {
            add_error(cur.line); // should not happen
            // try to recover
            return;
        }
        int decl_line = cur.line;
        if(cur.type != TokType::ID){
            add_error(cur.line);
            // try to skip until '('
            sync_until({TokType::LPAREN, TokType::LBRACE, TokType::SEMI, TokType::END});
            // recover best-effort
        }
        string fname;
        if(cur.type == TokType::ID) { fname = cur.lexeme; advance(); }
        // parameter list
        if(!expect(TokType::LPAREN)){
            // recover to '('
            sync_until({TokType::LPAREN, TokType::LBRACE, TokType::END});
            if(cur.type == TokType::LPAREN) advance();
        }
        vector<string> params;
        if(cur.type != TokType::RPAREN){
            // parse Param (',' Param)*
            while(true){
                if(cur.type == TokType::KW_INT){
                    advance();
                    if(cur.type == TokType::ID){
                        params.push_back(cur.lexeme);
                        advance();
                    } else {
                        add_error(cur.line);
                        // try to continue
                    }
                } else {
                    // missing "int" in param
                    add_error(cur.line);
                    // try to skip to next comma or ')'
                    sync_until({TokType::COMMA, TokType::RPAREN, TokType::END});
                }
                if(cur.type == TokType::COMMA) { advance(); continue; }
                else break;
            }
        }
        if(!expect(TokType::RPAREN)){
            // try to recover
            sync_until({TokType::LBRACE, TokType::SEMI, TokType::END});
            if(cur.type == TokType::RPAREN) advance();
        }
        // function decl uniqueness and order: functions must be declared in global scope and unique
        if(functions.count(fname)){
            // duplicate function name
            add_error(decl_line);
        } else {
            functions[fname] = {is_int, params, decl_line};
            if(fname == "main" && is_int && params.empty()) has_main = true;
        }
        // push new var scope for function body, add params
        var_scopes.emplace_back();
        for(auto &p: params) var_scopes.back()[p] = {true, decl_line};

        // parse Block
        parseBlock();

        // pop func scope
        var_scopes.pop_back();
    }

    // Block → “{” Stmt* “}”
    void parseBlock(){
        if(!expect(TokType::LBRACE)){
            // try to sync
            sync_until({TokType::RBRACE, TokType::END});
            if(cur.type == TokType::RBRACE) { advance(); return; }
            else return;
        }
        // new scope
        var_scopes.emplace_back();
        while(cur.type != TokType::RBRACE && cur.type != TokType::END){
            parseStmt();
        }
        if(!expect(TokType::RBRACE)){
            add_error(cur.line);
            // try to recover by popping scope
            // sync to next top-level
        }
        var_scopes.pop_back();
    }

    // Stmt → Block | “;” | Expr “;” | ID “=” Expr “;”
    //        | “int” ID “=” Expr “;”
    //        | “if ” “(” Expr “)” Stmt (“else” Stmt)?
    //        | “while” “(” Expr “)” Stmt
    //        | “break” “;” | “continue” “;” | “return” Expr “;”
    void parseStmt(){
        if(cur.type == TokType::LBRACE){ parseBlock(); return; }
        if(cur.type == TokType::SEMI){ advance(); return; }
        if(cur.type == TokType::KW_IF){
            advance();
            if(!expect(TokType::LPAREN)){
                add_error(cur.line);
                sync_until({TokType::RPAREN, TokType::LBRACE, TokType::SEMI, TokType::END});
            } else {
                // good
            }
            parseExpr();
            if(!expect(TokType::RPAREN)){
                add_error(cur.line);
            }
            parseStmt();
            if(cur.type == TokType::KW_ELSE){
                advance();
                parseStmt();
            }
            return;
        }
        if(cur.type == TokType::KW_WHILE){
            advance();
            if(!expect(TokType::LPAREN)){ add_error(cur.line); }
            in_loop = true;
            parseExpr();
            if(!expect(TokType::RPAREN)){ add_error(cur.line); }
            parseStmt();
            in_loop = false;
            return;
        }
        if(cur.type == TokType::KW_BREAK){
            int l = cur.line;
            advance();
            if(!expect(TokType::SEMI)) add_error(l);
            if(!in_loop) add_error(l);
            return;
        }
        if(cur.type == TokType::KW_CONTINUE){
            int l = cur.line;
            advance();
            if(!expect(TokType::SEMI)) add_error(l);
            if(!in_loop) add_error(l);
            return;
        }
        if(cur.type == TokType::KW_RETURN){
            int l = cur.line;
            advance();
            // return Expr ;
            parseExpr();
            if(!expect(TokType::SEMI)) add_error(l);
            // Note: matching return type to containing function is non-trivial here.
            return;
        }
        if(cur.type == TokType::KW_INT){
            // variable declaration: int ID = Expr ;
            int l = cur.line;
            advance();
            if(cur.type != TokType::ID){
                add_error(cur.line);
                // attempt recovery: skip to semicolon
                sync_until({TokType::SEMI, TokType::END});
                if(cur.type == TokType::SEMI) advance();
                return;
            }
            string var = cur.lexeme;
            int decl_line = cur.line;
            advance();
            if(!expect(TokType::ASSIGN)){
                add_error(cur.line);
                sync_until({TokType::SEMI, TokType::END});
                if(cur.type == TokType::SEMI) advance();
                // still insert var to avoid further undeclared issues
                var_scopes.back()[var] = {true, decl_line};
                return;
            }
            parseExpr();
            if(!expect(TokType::SEMI)){ add_error(cur.line); }
            // declare variable in current scope
            var_scopes.back()[var] = {true, decl_line};
            return;
        }
        // Could be assignment ID = Expr ; or Expr ;
        if(cur.type == TokType::ID){
            // lookahead: if next is '=' -> assignment; if next is '(' -> function call expression; else expression
            Token saved = cur;
            advance();
            if(cur.type == TokType::ASSIGN){
                // assignment
                string varname = saved.lexeme;
                int ln = cur.line;
                advance(); // =
                parseExpr();
                if(!expect(TokType::SEMI)) add_error(ln);
                // check var declared (scopes)
                if(!isVarDeclared(varname)){ add_error(saved.line); }
                return;
            } else {
                // not assignment: treat as start of expression
                // restore: (we can't un-read easily), so use a simple approach:
                // We will treat we consumed the ID as part of expression parse
                // Implement expression parsing functions to accept that cur is the token after ID.
                // To do that, we implement a small wrapper ExprFromLeadingId
                // For simplicity here: we put back by constructing a fake stream is hard; instead,
                // parse an expression with knowledge we already saw an ID: so call parseExprLeadingId(saved)
                parseExprLeadingId(saved);
                if(!expect(TokType::SEMI)) add_error(cur.line);
                return;
            }
        }
        // Other starting tokens: try parsing expression
        // For example starting with NUMBER, '(', +, -, !
        if(isExprStart(cur.type)){
            parseExpr();
            if(!expect(TokType::SEMI)) add_error(cur.line);
            return;
        }
        // unexpected token: record error and skip to next semicolon or brace
        add_error(cur.line);
        sync_until({TokType::SEMI, TokType::RBRACE, TokType::END});
        if(cur.type == TokType::SEMI) advance();
    }

    bool isExprStart(TokType t){
        return t==TokType::NUMBER || t==TokType::LPAREN || t==TokType::PLUS || t==TokType::MINUS || t==TokType::LNOT;
    }

    bool isVarDeclared(const string &name){
        for(int i=(int)var_scopes.size()-1;i>=0;--i){
            if(var_scopes[i].count(name)) return true;
        }
        return false;
    }

    // Expression parsing follows precedence
    // Expr -> LOrExpr
    void parseExpr(){ parseLOr(); }

    // Helper: parsing when we already consumed leading ID (was not assignment)
    void parseExprLeadingId(const Token &lead){
        // We treat "lead" as already consumed ID; construct behavior:
        // It could be: ID ( '(' args? ')' )? (binary ops...)
        // We'll do a small local parse: if next is LPAREN => function call primary, else variable primary.
        // parse remainder as if primary parsed.
        // create a faux primary parse by checking cur token
        // primary parsed: (we have ID in lead)
        if(cur.type == TokType::LPAREN){
            // function call: consume '(' ... ')'
            advance(); // consume '('
            if(cur.type != TokType::RPAREN){
                while(true){
                    parseExpr();
                    if(cur.type == TokType::COMMA) { advance(); continue; }
                    break;
                }
            }
            if(!expect(TokType::RPAREN)) add_error(cur.line);
        } else {
            // variable primary; check declared
            if(!isVarDeclared(lead.lexeme)){
                add_error(lead.line);
            }
        }
        // now parse binary expression tails using a simplified approach:
        parseBinaryTail();
    }

    // parse binary tails using a crude precedence climbing: since we've already handled left primary,
    // we allow sequences of binary ops. For simplicity, we use while loop over possible binary op tokens,
    // and parse the rhs via parseUnary().
    void parseBinaryTail(){
        while(true){
            if(isBinOp(cur.type)){
                advance(); // operator
                parseUnary();
            } else break;
        }
    }

    bool isBinOp(TokType t){
        switch(t){
            case TokType::PLUS: case TokType::MINUS:
            case TokType::MUL: case TokType::DIV: case TokType::MOD:
            case TokType::LT: case TokType::GT: case TokType::LE: case TokType::GE:
            case TokType::EQ: case TokType::NE:
            case TokType::LAND: case TokType::LOR:
                return true;
            default: return false;
        }
    }

    // parse LOrExpr → LAndExpr | LOrExpr “||” LAndExpr
    void parseLOr(){
        parseLAnd();
        while(cur.type == TokType::LOR){
            advance();
            parseLAnd();
        }
    }
    // LAndExpr → RelExpr | LAndExpr “&&” RelExpr
    void parseLAnd(){
        parseRel();
        while(cur.type == TokType::LAND){
            advance();
            parseRel();
        }
    }
    // RelExpr → AddExpr | RelExpr (relop) AddExpr
    void parseRel(){
        parseAdd();
        while(cur.type==TokType::LT || cur.type==TokType::GT || cur.type==TokType::LE || cur.type==TokType::GE || cur.type==TokType::EQ || cur.type==TokType::NE){
            advance();
            parseAdd();
        }
    }
    // AddExpr → MulExpr | AddExpr (“+” | “-”) MulExpr
    void parseAdd(){
        parseMul();
        while(cur.type==TokType::PLUS || cur.type==TokType::MINUS){
            advance();
            parseMul();
        }
    }
    // MulExpr → UnaryExpr | MulExpr (“*” | “/” | “%”) UnaryExpr
    void parseMul(){
        parseUnary();
        while(cur.type==TokType::MUL || cur.type==TokType::DIV || cur.type==TokType::MOD){
            advance();
            parseUnary();
        }
    }
    // UnaryExpr → PrimaryExpr | (“+” | “-” | “!”) UnaryExpr
    void parseUnary(){
        if(cur.type==TokType::PLUS || cur.type==TokType::MINUS || cur.type==TokType::LNOT){
            advance();
            parseUnary();
            return;
        }
        parsePrimary();
    }
    // PrimaryExpr → ID | NUMBER | “(” Expr “)” | ID “(” (Expr (',' Expr)*)? “)”
    void parsePrimary(){
        if(cur.type == TokType::ID){
            Token t = cur;
            advance();
            if(cur.type == TokType::LPAREN){
                // function call: consume '(' args ')'
                advance();
                if(cur.type != TokType::RPAREN){
                    while(true){
                        parseExpr();
                        if(cur.type == TokType::COMMA) { advance(); continue; }
                        break;
                    }
                }
                if(!expect(TokType::RPAREN)){
                    add_error(t.line);
                }
            } else {
                // variable reference
                if(!isVarDeclared(t.lexeme)){
                    add_error(t.line);
                }
            }
            return;
        } else if(cur.type == TokType::NUMBER){
            advance(); return;
        } else if(cur.type == TokType::LPAREN){
            advance();
            parseExpr();
            if(!expect(TokType::RPAREN)){
                add_error(cur.line);
            }
            return;
        } else {
            // unexpected primary
            add_error(cur.line);
            // attempt to recover: skip token
            advance();
            return;
        }
    }

    void finalize(){
        // outputs handled by caller
    }

    vector<int> get_errors() const { return error_lines; }
};


// --------------------------- main -----------------------------------------

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Parser parser(cin);
    parser.parseCompUnit();
    auto errs = parser.get_errors();
    if(errs.empty()){
        cout << "accept\n";
    } else {
        cout << "reject\n";
        for(int ln: errs) cout << ln << "\n";
    }
    return 0;
}

// toycc.cpp
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <queue>
#include <sstream>
using namespace std;

enum class TokType {
    END, ID, NUMBER,
    PLUS, MINUS, MUL, DIV, MOD,
    ASSIGN, SEMI, COMMA,
    LPAREN, RPAREN, LBRACE, RBRACE,
    LT, GT, LE, GE, EQ, NE,
    LAND, LOR, LNOT,
    KW_INT, KW_VOID, KW_IF, KW_ELSE, KW_WHILE, KW_BREAK, KW_CONTINUE, KW_RETURN,
    UNKNOWN
};

struct Token {
    TokType type;
    string lexeme;
    int line;
};

class Lexer {
    istream &in;
    int cur, line;
public:
    Lexer(istream &is): in(is), line(1) { cur = in.get(); }

    int peek() { return cur; }
    void consume() { cur = in.get(); if(cur=='\n') line++; }

    bool isIdentStart(int c){ return c=='_' || isalpha(c); }
    bool isIdentPart(int c){ return c=='_' || isalnum(c); }

    Token nextToken() {
        while(cur!=EOF && isspace(cur)) consume();

        if(cur=='/'){
            int next = in.peek();
            if(next=='/'){ consume(); consume(); while(cur!=EOF && cur!='\n') consume(); return nextToken(); }
            if(next=='*'){ consume(); consume(); while(cur!=EOF){ if(cur=='*'){ consume(); if(cur=='/'){ consume(); break; } } else consume(); } return nextToken(); }
        }

        Token tok; tok.line = line;
        if(cur==EOF || cur==-1){ tok.type=TokType::END; return tok; }

        if(isIdentStart(cur)){
            string s;
            while(cur!=EOF && isIdentPart(cur)){ s.push_back(cur); consume(); }
            tok.lexeme=s;
            if(s=="int") tok.type=TokType::KW_INT;
            else if(s=="void") tok.type=TokType::KW_VOID;
            else if(s=="if") tok.type=TokType::KW_IF;
            else if(s=="else") tok.type=TokType::KW_ELSE;
            else if(s=="while") tok.type=TokType::KW_WHILE;
            else if(s=="break") tok.type=TokType::KW_BREAK;
            else if(s=="continue") tok.type=TokType::KW_CONTINUE;
            else if(s=="return") tok.type=TokType::KW_RETURN;
            else tok.type=TokType::ID;
            return tok;
        }

        if(isdigit(cur)){
            string s; while(cur!=EOF && isdigit(cur)){ s.push_back(cur); consume(); }
            tok.type=TokType::NUMBER; tok.lexeme=s; return tok;
        }

        switch(cur){
            case '+': tok.type=TokType::PLUS; consume(); break;
            case '-': tok.type=TokType::MINUS; consume(); break;
            case '*': tok.type=TokType::MUL; consume(); break;
            case '/': tok.type=TokType::DIV; consume(); break;
            case '%': tok.type=TokType::MOD; consume(); break;
            case ';': tok.type=TokType::SEMI; consume(); break;
            case ',': tok.type=TokType::COMMA; consume(); break;
            case '(': tok.type=TokType::LPAREN; consume(); break;
            case ')': tok.type=TokType::RPAREN; consume(); break;
            case '{': tok.type=TokType::LBRACE; consume(); break;
            case '}': tok.type=TokType::RBRACE; consume(); break;
            case '!': consume(); if(cur=='='){ consume(); tok.type=TokType::NE; } else tok.type=TokType::LNOT; break;
            case '=': consume(); if(cur=='='){ consume(); tok.type=TokType::EQ; } else tok.type=TokType::ASSIGN; break;
            case '<': consume(); if(cur=='='){ consume(); tok.type=TokType::LE; } else tok.type=TokType::LT; break;
            case '>': consume(); if(cur=='='){ consume(); tok.type=TokType::GE; } else tok.type=TokType::GT; break;
            case '&': consume(); if(cur=='&'){ consume(); tok.type=TokType::LAND; } else tok.type=TokType::UNKNOWN; break;
            case '|': consume(); if(cur=='|'){ consume(); tok.type=TokType::LOR; } else tok.type=TokType::UNKNOWN; break;
            default: tok.type=TokType::UNKNOWN; consume(); break;
        }
        return tok;
    }
};

class Parser {
    Lexer lex;
    Token cur;
    vector<int> errors;
    set<int> error_set;

    map<string,bool> functions_int;
    map<string,int> functions_line;
    vector<map<string,int>> var_scopes;

    bool in_loop = false;
    bool has_main = false;

public:
    Parser(istream &is): lex(is){ cur=lex.nextToken(); }

    void add_error(int ln){ if(!error_set.count(ln)){ errors.push_back(ln); error_set.insert(ln); } }
    void advance(){ cur=lex.nextToken(); }
    bool accept(TokType t){ if(cur.type==t){ advance(); return true;} return false; }
    bool expect(TokType t, int stmt_line){ if(cur.type==t){ advance(); return true;} add_error(stmt_line); return false; }

    bool isVarDeclared(const string &name){
        for(int i=(int)var_scopes.size()-1;i>=0;i--) if(var_scopes[i].count(name)) return true;
        return false;
    }

    void parseCompUnit(){
        while(cur.type!=TokType::END){
            if(cur.type==TokType::KW_INT || cur.type==TokType::KW_VOID) {
                var_scopes.clear();
                in_loop = false;
                parseFuncDef();
            }
            else { add_error(cur.line); while(cur.type!=TokType::KW_INT && cur.type!=TokType::KW_VOID && cur.type!=TokType::END) advance();}
        }
        if(!has_main) add_error(1);
    }

    void parseFuncDef(){
        bool is_int = (cur.type==TokType::KW_INT); 
        int func_line = cur.line;
        advance();
        int decl_line = cur.line;
        string fname;
        if(cur.type==TokType::ID){ fname=cur.lexeme; advance(); }
        else{ add_error(decl_line); sync_until({TokType::LPAREN, TokType::LBRACE, TokType::SEMI, TokType::END}); if(cur.type==TokType::ID) { fname=cur.lexeme; advance(); } }
        
        int lparen_line = cur.line;
        expect(TokType::LPAREN, lparen_line);
        vector<string> params;
        if(cur.type!=TokType::RPAREN){
            while(true){
                if(cur.type==TokType::KW_INT){ advance(); if(cur.type==TokType::ID){ params.push_back(cur.lexeme); advance(); } else add_error(cur.line);}
                else add_error(cur.line);
                if(cur.type==TokType::COMMA) advance(); else break;
            }
        }
        expect(TokType::RPAREN, lparen_line);
        if(functions_int.count(fname)) add_error(decl_line);
        else{ functions_int[fname]=is_int; functions_line[fname]=decl_line; }
        if(fname=="main" && is_int && params.empty()) has_main=true;

        int scope_size_before = var_scopes.size();
        
        var_scopes.emplace_back();
        for(auto &p: params) var_scopes.back()[p]=decl_line;

        parseBlock();

        while((int)var_scopes.size() > scope_size_before) {
            var_scopes.pop_back();
        }
        
        in_loop = false;
    }

    bool parseBlock(){
        int block_line = cur.line;
        if(!expect(TokType::LBRACE, block_line)) return false;
        var_scopes.emplace_back();
        while(cur.type!=TokType::RBRACE && cur.type!=TokType::END && 
              cur.type!=TokType::KW_VOID) {
            parseStmt();
        }
        if(cur.type==TokType::KW_VOID) {
            add_error(cur.line);
            var_scopes.pop_back();
            return false;
        }
        bool rbrace_ok = expect(TokType::RBRACE, block_line);
        var_scopes.pop_back();
        return rbrace_ok;
    }

    void parseStmt(){
        int stmt_line = cur.line;
        if(cur.type==TokType::LBRACE){ parseBlock(); return; }
        if(cur.type==TokType::SEMI){ advance(); return; }

        if(cur.type==TokType::KW_IF){
            int if_line = cur.line;
            advance(); 
            expect(TokType::LPAREN, if_line); 
            parseExpr(stmt_line); 
            expect(TokType::RPAREN, if_line); 
            parseStmt();
            if(cur.type==TokType::KW_ELSE){ advance(); parseStmt(); }
            return;
        }
        if(cur.type==TokType::KW_WHILE){
            int while_line = cur.line;
            advance(); 
            expect(TokType::LPAREN, while_line); 
            bool old_loop=in_loop; 
            in_loop=true; 
            parseExpr(stmt_line); 
            expect(TokType::RPAREN, while_line); 
            parseStmt(); 
            in_loop=old_loop; 
            return;
        }
        if(cur.type==TokType::KW_BREAK){ advance(); expect(TokType::SEMI, stmt_line); if(!in_loop) add_error(stmt_line); return; }
        if(cur.type==TokType::KW_CONTINUE){ advance(); expect(TokType::SEMI, stmt_line); if(!in_loop) add_error(stmt_line); return; }
        if(cur.type==TokType::KW_RETURN){ advance(); parseExpr(stmt_line); expect(TokType::SEMI, stmt_line); return; }

        if(cur.type==TokType::KW_INT){
            advance();
            if(cur.type!=TokType::ID){ add_error(stmt_line); sync_until({TokType::SEMI}); if(cur.type==TokType::SEMI) advance(); return; }
            string var = cur.lexeme; advance();
            expect(TokType::ASSIGN, stmt_line); parseExpr(stmt_line); expect(TokType::SEMI, stmt_line);
            var_scopes.back()[var]=stmt_line; return;
        }

        if(cur.type==TokType::ID){
            Token saved=cur; advance();
            if(cur.type==TokType::ASSIGN){ advance(); parseExpr(stmt_line); expect(TokType::SEMI, stmt_line); if(!isVarDeclared(saved.lexeme)) add_error(stmt_line); return; }
            else{ parseExprLeadingId(saved, stmt_line); expect(TokType::SEMI, stmt_line); return; }
        }

        if(isExprStart(cur.type)){ parseExpr(stmt_line); expect(TokType::SEMI, stmt_line); return; }

        add_error(stmt_line); sync_until({TokType::SEMI, TokType::RBRACE, TokType::END}); if(cur.type==TokType::SEMI) advance();
    }

    void parseExpr(int stmt_line){ parseLOr(stmt_line); }

    void parseExprLeadingId(Token &lead, int stmt_line){
        if(cur.type==TokType::LPAREN){ 
            int call_line = lead.line;
            advance(); 
            if(cur.type!=TokType::RPAREN){ 
                while(true){ 
                    parseExpr(stmt_line); 
                    if(cur.type==TokType::COMMA) advance(); 
                    else break; 
                } 
            } 
            expect(TokType::RPAREN, call_line); 
        }
        else{ if(!isVarDeclared(lead.lexeme)) add_error(stmt_line); }
        parseBinaryTail(stmt_line);
    }

    void parseBinaryTail(int stmt_line){ while(isBinOp(cur.type)){ advance(); parseUnary(stmt_line); } }
    bool isBinOp(TokType t){ switch(t){ case TokType::PLUS: case TokType::MINUS: case TokType::MUL: case TokType::DIV: case TokType::MOD: case TokType::LT: case TokType::GT: case TokType::LE: case TokType::GE: case TokType::EQ: case TokType::NE: case TokType::LAND: case TokType::LOR: return true; default: return false; } }

    void parseLOr(int stmt_line){ parseLAnd(stmt_line); while(cur.type==TokType::LOR){ advance(); parseLAnd(stmt_line); } }
    void parseLAnd(int stmt_line){ parseRel(stmt_line); while(cur.type==TokType::LAND){ advance(); parseRel(stmt_line); } }
    void parseRel(int stmt_line){ parseAdd(stmt_line); while(cur.type==TokType::LT || cur.type==TokType::GT || cur.type==TokType::LE || cur.type==TokType::GE || cur.type==TokType::EQ || cur.type==TokType::NE){ advance(); parseAdd(stmt_line); } }
    void parseAdd(int stmt_line){ parseMul(stmt_line); while(cur.type==TokType::PLUS || cur.type==TokType::MINUS){ advance(); parseMul(stmt_line); } }
    void parseMul(int stmt_line){ parseUnary(stmt_line); while(cur.type==TokType::MUL || cur.type==TokType::DIV || cur.type==TokType::MOD){ advance(); parseUnary(stmt_line); } }
    void parseUnary(int stmt_line){ if(cur.type==TokType::PLUS || cur.type==TokType::MINUS || cur.type==TokType::LNOT){ advance(); parseUnary(stmt_line); return;} parsePrimary(stmt_line); }
    void parsePrimary(int stmt_line){
        if(cur.type==TokType::ID){ 
            Token t=cur; 
            int id_line = cur.line;
            advance();
            if(cur.type==TokType::LPAREN){ 
                advance(); 
                if(cur.type!=TokType::RPAREN){ 
                    while(true){ 
                        parseExpr(stmt_line); 
                        if(cur.type==TokType::COMMA) advance(); 
                        else break; 
                    } 
                } 
                expect(TokType::RPAREN, id_line);
            }
            else if(!isVarDeclared(t.lexeme)) add_error(stmt_line);
            return;
        } else if(cur.type==TokType::NUMBER){ advance(); return;}
        else if(cur.type==TokType::LPAREN){ 
            int lparen_line = cur.line;
            advance(); 
            parseExpr(stmt_line); 
            expect(TokType::RPAREN, lparen_line); 
            return;
        }
        else{ add_error(stmt_line); advance(); return;}
    }

    bool isExprStart(TokType t){ return t==TokType::NUMBER || t==TokType::LPAREN || t==TokType::PLUS || t==TokType::MINUS || t==TokType::LNOT; }

    void sync_until(set<TokType> syncset){ int iter=0; while(cur.type!=TokType::END && !syncset.count(cur.type) && iter<100000){ advance(); ++iter; } }

    vector<int> get_errors(){ return errors; }
};

int main(){
    ios::sync_with_stdio(false); cin.tie(nullptr);
    
    // 读取所有输入到字符串
    string input_content;
    string line;
    while(getline(cin, line)) {
        input_content += line + "\n";
    }
    
    // 输出输入内容
    cout << input_content;
    
    // 使用stringstream进行解析
    istringstream iss(input_content);
    Parser p(iss);
    p.parseCompUnit();
    auto errs = p.get_errors();
    
    if(errs.empty()){ 
        cout<<"accept\n"; 
    }
    else{ 
        cout<<"reject\n"; 
        for(int ln: errs) cout<<ln<<"\n"; 
    }
}
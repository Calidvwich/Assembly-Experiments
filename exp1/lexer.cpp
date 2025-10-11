#include <iostream>
#include <string>
#include <cctype>
#include <vector>
#include <unordered_set>

using namespace std;

struct Token {
    int index;
    string type;
    string value;
};

// 关键字集合
unordered_set<string> keywords = {
    "int", "void", "if", "else", "while", "break", "continue", "return"
};

// 双字符运算符
unordered_set<string> doubleOps = {"==", "!=", "<=", ">=", "&&", "||"};

// 单字符运算符
unordered_set<char> singleOps = {
    '+','-','*','/','%','<','>','=','!','(',')','{','}',';',','
};

bool isIdentStart(char c) {
    return isalpha(c) || c == '_';
}

bool isIdentPart(char c) {
    return isalnum(c) || c == '_';
}

bool isDigit(char c) {
    return isdigit(c);
}

// 跳过空白和注释
void skipWhitespaceAndComments(string &code, size_t &pos) {
    while (pos < code.size()) {
        if (isspace(code[pos])) {
            ++pos;
        } else if (code[pos] == '/' && pos + 1 < code.size()) {
            if (code[pos + 1] == '/') {
                // 单行注释
                pos += 2;
                while (pos < code.size() && code[pos] != '\n') ++pos;
            } else if (code[pos + 1] == '*') {
                // 多行注释
                pos += 2;
                while (pos + 1 < code.size() && !(code[pos] == '*' && code[pos + 1] == '/')) ++pos;
                if (pos + 1 < code.size()) pos += 2;
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

vector<Token> tokenize(const string &code) {
    vector<Token> tokens;
    size_t pos = 0;
    int index = 0;

    while (pos < code.size()) {
        skipWhitespaceAndComments(const_cast<string&>(code), pos);
        if (pos >= code.size()) break;

        char c = code[pos];

        // 关键字/标识符
        if (isIdentStart(c)) {
            size_t start = pos;
            while (pos < code.size() && isIdentPart(code[pos])) ++pos;
            string word = code.substr(start, pos - start);
            if (keywords.count(word)) {
                tokens.push_back({index++, "'" + word + "'", "\"" + word + "\""});
            } else {
                tokens.push_back({index++, "Ident", "\"" + word + "\""});
            }
        }
        // 数字
        else if (isDigit(c) || (c == '-' && pos + 1 < code.size() && isDigit(code[pos+1]))) {
            size_t start = pos;
            if (c == '-') ++pos;
            while (pos < code.size() && isDigit(code[pos])) ++pos;
            string num = code.substr(start, pos - start);
            tokens.push_back({index++, "IntConst", "\"" + num + "\""});
        }
        // 双字符运算符
        else if (pos + 1 < code.size() && doubleOps.count(code.substr(pos,2))) {
            string op = code.substr(pos,2);
            tokens.push_back({index++, "'" + op + "'", "\"" + op + "\""});
            pos += 2;
        }
        // 单字符运算符或分隔符
        else if (singleOps.count(c)) {
            string op(1, c);
            tokens.push_back({index++, "'" + op + "'", "\"" + op + "\""});
            ++pos;
        }
        else {
            cerr << "Unknown character: " << c << " at position " << pos << endl;
            ++pos; // 避免死循环
        }
    }

    return tokens;
}

int main() {
    string code, line;
    while (getline(cin, line)) {
        code += line + '\n';
    }

    vector<Token> tokens = tokenize(code);
    for (auto &t : tokens) {
        cout << t.index << ":" << t.type << ":" << t.value << endl;
    }
    return 0;
}

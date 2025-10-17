#include <iostream>
#include <string>
#include <cctype>
#include <vector>
#include <unordered_set>

using namespace std;
// 定义Token结构体。一个token应该包含序号index，类型type以及具体的内容value
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
// 判断标识符首字母，按照题目要求，只能是字母or_
bool isIdentStart(char c) {
    return isalpha(c) || c == '_';
}
// 判断标识符的其他部分，按照题目要求只能是字母or数字or_
bool isIdentPart(char c) {
    return isalnum(c) || c == '_';
}
// 判断是不是数字
bool isDigit(char c) {
    return isdigit(c);
}
// 跳过空白和注释
void skipWhitespaceAndComments(string &code, size_t &pos) {
    while (pos < code.size()) {
        // 跳过空格
        if (isspace(code[pos])) {
            ++pos;
        }
        // 跳过注释。无论是多行注释还是单行注释一定是/开头，因此读取到/时开始检测。 
        else if (code[pos] == '/' && pos + 1 < code.size()) {
            if (code[pos + 1] == '/') {
                // 跳过单行注释，检测到//时，该行是单行注释，遍历到换行（本行结束）即可
                pos += 2;
                while (pos < code.size() && code[pos] != '\n') ++pos;
            } else if (code[pos + 1] == '*') {
                // 跳过多行注释，检测到/*时，一定是多行注释。因此只需要不断循环直到*/连续出现，此时多行注释结束
                pos += 2;
                while (pos + 1 < code.size() && !(code[pos] == '*' && code[pos + 1] == '/')) ++pos;
                if (pos + 1 < code.size()) pos += 2;
            } else {
                break;// 不是上述情况，忽略。
            }
        } else {
            break;
        }
    }
}
// 词法分析主函数。思路如下：
// 遍历整个代码的时候，重复调用上面的函数跳过注释和空格。
// 确认不是上述可忽略情况后，则对象一定是一个token，判断类型即可
vector<Token> tokenize(const string &code) {
    vector<Token> tokens;
    size_t pos = 0;
    int index = 0;

    while (pos < code.size()) {
        // 先决判断是否可忽略。
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
            ++pos; // 避免死循环，遇到问题则抛出错误
        }
    }

    return tokens;
}
// 主函数部分
// 参考规则，test.c文件中未结束则持续判断类型，换行输出元素类型和内容。格式为id+类型+数值
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

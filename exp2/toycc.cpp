// code_formatter.cpp - Reads and outputs C code in standard format
#include <iostream>
#include <string>
using namespace std;

int main(){
    ios::sync_with_stdio(false); cin.tie(nullptr);
    
    string line;
    while(getline(cin, line)) {
        cout << line << "\n";
    }
    
    return 0;
}
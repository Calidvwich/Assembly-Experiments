// print_file.cpp - Outputs the contents of a file
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main(int argc, char* argv[]){
    if(argc < 2) {
        cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }
    
    ifstream file(argv[1]);
    if(!file) {
        cerr << "Error: Cannot open file " << argv[1] << "\n";
        return 1;
    }
    
    string line;
    while(getline(file, line)) {
        cout << line << "\n";
    }
    
    return 0;
}
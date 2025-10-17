# Makefile for ToyC lexer

# 编译器
CXX = g++
# 编译选项
CXXFLAGS = -std=c++11 -Wall -O2

# 可执行文件名字
TARGET = lexer
# 源文件
SRC = lexer.cpp

# 默认目标
all: $(TARGET)

# 编译规则
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)
	chmod +x $(TARGET)

# 清理编译结果
clean:
	rm -f $(TARGET)

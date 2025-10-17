# Makefile for ToyC lexer

CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2

TARGET = toyc
SRC = lexer.cpp

# 默认目标
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# 清理
clean:
	rm -f $(TARGET)

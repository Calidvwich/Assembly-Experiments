# 编译器与参数
CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -g

# 源文件与目标文件
SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)
TARGET := toycc

# 默认目标
all: $(TARGET)

# 链接规则
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 编译规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f *.o $(TARGET)

# 伪目标
.PHONY: all clean

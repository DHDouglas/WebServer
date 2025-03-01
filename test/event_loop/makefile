CXX := g++
CXXFLAGS := -std=c++14 -Wall -g

# 项目根目录
ROOT = /home/yht/Projects/MyTinyWebServer

# 指定头文件目录
INCLUDES = ${ROOT}/src/include
CXXFLAGS += $(addprefix -I, $(INCLUDES)) 
# 指定源文件目录
SRC_DIRS := ${ROOT}/src/channel ${ROOT}/src/epoller ${ROOT}/src/eventloop ${ROOT}/src/timer
# 获取所有.cpp源文件
SRCS := $(wildcard $(addsuffix /*.cpp, $(SRC_DIRS)))
MAIN ?= test
SRCS += $(addsuffix .cpp, $(MAIN))

# 需对应生成的.o目标文件
#OBJS := $(SRCS:.cpp=.o)

# 目标可执行文件
EXEC := MAIN

$(EXEC): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

#%.o: %.cpp
#	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJS)

.PHONY: clean
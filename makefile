CXX := g++
CXXFLAGS := -std=c++14 -Wall -g

# 
ROOT_PATH = ~/Projects/MyTinyWebServer

# 指定头文件目录
INCLUDES := ./src/include
CXXFLAGS += $(addprefix -I, $(INCLUDES)) 
# 指定源文件目录
SRC_DIRS := ./src/channel ./src/epoller ./src/eventloop
# 获取所有.cpp源文件
SRCS := $(wildcard $(addsuffix /*.cpp, $(SRC_DIRS)))
# 需对应生成的.o目标文件
#OBJS := $(SRCS:.cpp=.o)

# 目标可执行文件
EXEC := a.out


$(EXEC): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

#%.o: %.cpp
#	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJS)

.PHONY: clean
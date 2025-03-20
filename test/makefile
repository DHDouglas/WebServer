CXX := g++
CXXFLAGS := -std=c++14 -Wall -g

# 项目根目录
ROOT := /home/yht/Projects/MyTinyWebServer
ROOT_SRC_DIR := ${ROOT}/src

# 指定头文件目录
INCLUDES = ${ROOT_SRC_DIR}/include
CXXFLAGS += $(addprefix -I, $(INCLUDES)) 
# 指定源文件目录
SRC_DIRS := ${ROOT_SRC_DIR}/channel ${ROOT_SRC_DIR}/epoller ${ROOT_SRC_DIR}/eventloop ${ROOT_SRC_DIR}/timer
SRC_DIRS += ${ROOT_SRC_DIR}/acceptor ${ROOT_SRC_DIR}/base ${ROOT_SRC_DIR}/logger
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

test_thread: test_thread.cpp ${ROOT_SRC_DIR}/base/thread.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

LOGGER_SRCS := $(wildcard $(addsuffix /*.cpp, $(ROOT_SRC_DIR)/logger))

test_logger: test_logger.cpp ${LOGGER_SRCS} ${ROOT_SRC_DIR}/base/thread.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_logfile: test_logfile.cpp ${LOGGER_SRCS} ${ROOT_SRC_DIR}/base/thread.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
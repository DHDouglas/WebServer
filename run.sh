#!/bin/bash

PROGRAM="./build/Debug/bin/http_server"

IP=""                        # 监听地址. 为空时监听所有地址.
PORT=8080                    # 监听端口.
THREAD_NUM=5                 # IO线程数量. 即subReactor数量, 为0则主线程兼做IO线程.
ROOT_PATH="./resources"      # Web资源文件根路径.
TIMEOUT=30                   # HttpConnection 超时时间.
LOG_FNAME="HttpServerLog"    # 日志文件名. 日志文件路径默认为cwd.
LOG_DIR="./log"              # 日志目录
LOG_LEVEL=2                  # 日志级别. 0:TRACE, 1:DEBUG, 2:INFO, 3:WARN, 4:ERROR, 5:FATAL.
LOG_ROLLSIZE=500000000       # 单份日志文件字节上限(写满后切换新文件). 0.5GB.
LOG_FLUSH_TIME=2             # 从buffer定期刷入日志文件的间隔秒数. 2秒.


ARGS=()
if [[ -n "$IP" ]]; then
    ARGS+=("-i=$IP")
fi
ARGS+=("-p" "$PORT")
ARGS+=("-j" "$THREAD_NUM")
ARGS+=("-r" "$ROOT_PATH")
ARGS+=("-t" "$TIMEOUT")
ARGS+=("-f" "$LOG_FNAME")
ARGS+=("-R" "$LOG_DIR")
ARGS+=("-l" "$LOG_LEVEL")
ARGS+=("-s" "$LOG_ROLLSIZE")
ARGS+=("-u" "$LOG_FLUSH_TIME")

$PROGRAM "${ARGS[@]}" 

# $PROGRAM "-i $IP -p $PORT -j $THREAD_NUM -r $ROOT_PATH -t $TIMEOUT \
#     -f $LOG_FNAME -R $LOG_DIR -l $LOG_LEVEL -s $LOG_ROLLSIZE -u $LOG_FLUSH_TIME"

# ./build/Release/bin/http_server -p 8080 -j 5 -r "./recources" -t 30 -f "HttpServerLog" -R "./log" -l 2 -s 500000000 -u 2
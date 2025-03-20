#!/bin/bash

# 默认参数
THREADS=4           # wrk的线程数
CONNECTIONS=1000    # wrk维持的并发连接数
DURATION=60         # wrk压测持续时间(秒)
PORT=80             
URL="/benchmark"
KEEP_ALIVE=false    # 是否长连接

# 解析命令行参数
while getopts "t:c:d:p:u:k" opt; do
    case ${opt} in
        t) THREADS="$OPTARG" ;;
        c) CONNECTIONS="$OPTARG" ;;
        d) DURATION="$OPTARG" ;;
        p) PORT="$OPTARG" ;;
        u) URL="$OPTARG" ;;
        k) KEEP_ALIVE=true ;;
        *) echo "Usage: $0 [-t threads] [-c connections] [-d duration] [-p port] [-u url] [-k (keep-alive)]"; exit 1 ;;
    esac
done

if [[ "$KEEP_ALIVE" == true ]]; then
    CMD="wrk -t${THREADS} -c${CONNECTIONS} -d${DURATION}s http://localhost:"${PORT}${URL}""
else
    CMD="wrk -t${THREADS} -c${CONNECTIONS} -d${DURATION}s --header \"Connection: close\" http://localhost:${PORT}${URL}"
fi

echo "CMD: $CMD"
eval "$CMD"

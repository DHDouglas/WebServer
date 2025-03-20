# Diff Details

Date : 2025-03-20 00:16:31

Directory /home/yht/Projects/MyTinyWebServer/test

Total : 76 files,  -2700 codes, -107 comments, -554 blanks, all -3361 lines

[Summary](results.md) / [Details](details.md) / [Diff Summary](diff.md) / Diff Details

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [src/CMakeLists.txt](/src/CMakeLists.txt) | CMake | -52 | 0 | -8 | -60 |
| [src/acceptor/acceptor.cpp](/src/acceptor/acceptor.cpp) | C++ | -102 | -7 | -14 | -123 |
| [src/acceptor/acceptor.h](/src/acceptor/acceptor.h) | C++ | -26 | 0 | -6 | -32 |
| [src/base/any.h](/src/base/any.h) | C++ | -51 | -4 | -17 | -72 |
| [src/base/config.cpp](/src/base/config.cpp) | C++ | -118 | -4 | -12 | -134 |
| [src/base/config.h](/src/base/config.h) | C++ | -28 | -12 | -6 | -46 |
| [src/base/count\_down\_latch.h](/src/base/count_down_latch.h) | C++ | -32 | 0 | -8 | -40 |
| [src/base/inet\_address.cpp](/src/base/inet_address.cpp) | C++ | -100 | 0 | -20 | -120 |
| [src/base/inet\_address.h](/src/base/inet_address.h) | C++ | -31 | 0 | -8 | -39 |
| [src/base/thread.cpp](/src/base/thread.cpp) | C++ | -86 | -3 | -21 | -110 |
| [src/base/thread.h](/src/base/thread.h) | C++ | -44 | -4 | -19 | -67 |
| [src/buffer/buffer.cpp](/src/buffer/buffer.cpp) | C++ | -131 | -7 | -35 | -173 |
| [src/buffer/buffer.h](/src/buffer/buffer.h) | C++ | -38 | -26 | -16 | -80 |
| [src/channel/channel.cpp](/src/channel/channel.cpp) | C++ | -138 | -1 | -39 | -178 |
| [src/channel/channel.h](/src/channel/channel.h) | C++ | -58 | -4 | -12 | -74 |
| [src/epoller/epoller.cpp](/src/epoller/epoller.cpp) | C++ | -133 | -2 | -20 | -155 |
| [src/epoller/epoller.h](/src/epoller/epoller.h) | C++ | -30 | 0 | -11 | -41 |
| [src/eventloop/eventloop.cpp](/src/eventloop/eventloop.cpp) | C++ | -161 | -2 | -40 | -203 |
| [src/eventloop/eventloop.h](/src/eventloop/eventloop.h) | C++ | -59 | -3 | -18 | -80 |
| [src/eventloop/eventloop\_thread.cpp](/src/eventloop/eventloop_thread.cpp) | C++ | -42 | -4 | -9 | -55 |
| [src/eventloop/eventloop\_thread.h](/src/eventloop/eventloop_thread.h) | C++ | -23 | 0 | -8 | -31 |
| [src/eventloop/eventloop\_threadpool.cpp](/src/eventloop/eventloop_threadpool.cpp) | C++ | -39 | -1 | -9 | -49 |
| [src/eventloop/eventloop\_threadpool.h](/src/eventloop/eventloop_threadpool.h) | C++ | -26 | -1 | -9 | -36 |
| [src/http/http\_connection.cpp](/src/http/http_connection.cpp) | C++ | -272 | -20 | -35 | -327 |
| [src/http/http\_connection.h](/src/http/http_connection.h) | C++ | -48 | -11 | -13 | -72 |
| [src/http/http\_message.cpp](/src/http/http_message.cpp) | C++ | -234 | -7 | -42 | -283 |
| [src/http/http\_message.h](/src/http/http_message.h) | C++ | -109 | -1 | -27 | -137 |
| [src/http/http\_parser.cpp](/src/http/http_parser.cpp) | C++ | -313 | -14 | -54 | -381 |
| [src/http/http\_parser.h](/src/http/http_parser.h) | C++ | -83 | 0 | -14 | -97 |
| [src/http/http\_server.cpp](/src/http/http_server.cpp) | C++ | -87 | -8 | -14 | -109 |
| [src/http/http\_server.h](/src/http/http_server.h) | C++ | -27 | -2 | -10 | -39 |
| [src/logger/async\_logger.cpp](/src/logger/async_logger.cpp) | C++ | -118 | -7 | -17 | -142 |
| [src/logger/async\_logger.h](/src/logger/async_logger.h) | C++ | -35 | -1 | -6 | -42 |
| [src/logger/log\_file.cpp](/src/logger/log_file.cpp) | C++ | -157 | -6 | -27 | -190 |
| [src/logger/log\_file.h](/src/logger/log_file.h) | C++ | -49 | -2 | -13 | -64 |
| [src/logger/log\_stream.cpp](/src/logger/log_stream.cpp) | C++ | -144 | -3 | -34 | -181 |
| [src/logger/log\_stream.h](/src/logger/log_stream.h) | C++ | -69 | -3 | -20 | -92 |
| [src/logger/logger.cpp](/src/logger/logger.cpp) | C++ | -96 | -7 | -26 | -129 |
| [src/logger/logger.h](/src/logger/logger.h) | C++ | -90 | -7 | -23 | -120 |
| [src/main.cpp](/src/main.cpp) | C++ | -6 | 0 | -1 | -7 |
| [src/tcp/tcp\_connection.cpp](/src/tcp/tcp_connection.cpp) | C++ | -311 | -33 | -38 | -382 |
| [src/tcp/tcp\_connection.h](/src/tcp/tcp_connection.h) | C++ | -73 | -11 | -23 | -107 |
| [src/tcp/tcp\_server.cpp](/src/tcp/tcp_server.cpp) | C++ | -72 | -11 | -21 | -104 |
| [src/tcp/tcp\_server.h](/src/tcp/tcp_server.h) | C++ | -50 | -18 | -18 | -86 |
| [src/timer/timeing\_wheel.cpp](/src/timer/timeing_wheel.cpp) | C++ | -70 | -9 | -16 | -95 |
| [src/timer/timer.cpp](/src/timer/timer.cpp) | C++ | -111 | -13 | -19 | -143 |
| [src/timer/timer.h](/src/timer/timer.h) | C++ | -55 | -1 | -15 | -71 |
| [src/timer/timestamp.h](/src/timer/timestamp.h) | C++ | -83 | -4 | -19 | -106 |
| [src/timer/timing\_wheel.h](/src/timer/timing_wheel.h) | C++ | -44 | 0 | -8 | -52 |
| [test/CMakeLists.txt](/test/CMakeLists.txt) | CMake | 85 | 0 | 26 | 111 |
| [test/acceptor\_test.cpp](/test/acceptor_test.cpp) | C++ | 24 | 0 | 5 | 29 |
| [test/any\_class\_test.cpp](/test/any_class_test.cpp) | C++ | 45 | 0 | 5 | 50 |
| [test/asynclogging\_test.cpp](/test/asynclogging_test.cpp) | C++ | 28 | 3 | 10 | 41 |
| [test/buffer\_unittest.cpp](/test/buffer_unittest.cpp) | C++ | 63 | 73 | 29 | 165 |
| [test/channel\_epoller\_test.cpp](/test/channel_epoller_test.cpp) | C++ | 30 | 3 | 9 | 42 |
| [test/config\_test.cpp](/test/config_test.cpp) | C++ | 6 | 0 | 2 | 8 |
| [test/eventloop2\_test.cpp](/test/eventloop2_test.cpp) | C++ | 12 | 1 | 5 | 18 |
| [test/eventloop\_test.cpp](/test/eventloop_test.cpp) | C++ | 19 | 0 | 3 | 22 |
| [test/eventloop\_thread\_test.cpp](/test/eventloop_thread_test.cpp) | C++ | 24 | 1 | 6 | 31 |
| [test/http\_parser\_unittest.cpp](/test/http_parser_unittest.cpp) | C++ | 163 | 24 | 33 | 220 |
| [test/http\_parser\_with\_tcp\_server\_test.cpp](/test/http_parser_with_tcp_server_test.cpp) | C++ | 49 | 2 | 12 | 63 |
| [test/http\_server\_unittest.cpp](/test/http_server_unittest.cpp) | C++ | 547 | 35 | 82 | 664 |
| [test/httpserver\_demo\_test.cpp](/test/httpserver_demo_test.cpp) | C++ | 6 | 0 | 1 | 7 |
| [test/inet\_address\_unittest.cpp](/test/inet_address_unittest.cpp) | C++ | 78 | 5 | 29 | 112 |
| [test/logfile\_test.cpp](/test/logfile_test.cpp) | C++ | 27 | 0 | 9 | 36 |
| [test/logger\_test.cpp](/test/logger_test.cpp) | C++ | 39 | 0 | 6 | 45 |
| [test/makefile](/test/makefile) | Makefile | 24 | 9 | 12 | 45 |
| [test/pending\_functor\_in\_loop\_test.cpp](/test/pending_functor_in_loop_test.cpp) | C++ | 32 | 1 | 10 | 43 |
| [test/tcpconn\_send2\_test.cpp](/test/tcpconn_send2_test.cpp) | C++ | 47 | 1 | 9 | 57 |
| [test/tcpconn\_send3\_test.cpp](/test/tcpconn_send3_test.cpp) | C++ | 51 | 1 | 9 | 61 |
| [test/tcpconn\_send\_test.cpp](/test/tcpconn_send_test.cpp) | C++ | 43 | 1 | 9 | 53 |
| [test/tcpserver\_test.cpp](/test/tcpserver_test.cpp) | C++ | 41 | 1 | 8 | 50 |
| [test/thread\_test.cpp](/test/thread_test.cpp) | C++ | 17 | 5 | 5 | 27 |
| [test/timer\_self\_cancel\_test.cpp](/test/timer_self_cancel_test.cpp) | C++ | 28 | 0 | 5 | 33 |
| [test/timer\_test.cpp](/test/timer_test.cpp) | C++ | 34 | 0 | 7 | 41 |
| [test/timewheel\_test.cpp](/test/timewheel_test.cpp) | C++ | 92 | 11 | 18 | 121 |

[Summary](results.md) / [Details](details.md) / [Diff Summary](diff.md) / Diff Details
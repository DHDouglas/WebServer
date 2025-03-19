add_executable(eventloop1_test eventloop_test.cpp)
target_link_libraries(eventloop1_test PRIVATE MyObjects)

add_executable(eventloop2_test eventloop2_test.cpp)
target_link_libraries(eventloop2_test PRIVATE MyObjects)

add_executable(channel_epoller_test channel_epoller_test.cpp)
target_link_libraries(channel_epoller_test PRIVATE MyObjects)

add_executable(thread_test thread_test.cpp)
target_link_libraries(thread_test PRIVATE MyObjects)

add_executable(timer_test timer_test.cpp)
target_link_libraries(timer_test PRIVATE MyObjects)

add_executable(timer_self_cancel_test timer_self_cancel_test.cpp)
target_link_libraries(timer_self_cancel_test PRIVATE MyObjects)

add_executable(pending_functor_in_loop_test pending_functor_in_loop_test.cpp)
target_link_libraries(pending_functor_in_loop_test PRIVATE MyObjects)

add_executable(eventloop_thread_test eventloop_thread_test.cpp)
target_link_libraries(eventloop_thread_test PRIVATE MyObjects)

add_executable(acceptor_test acceptor_test.cpp)
target_link_libraries(acceptor_test PRIVATE MyObjects)

add_executable(logfile_test logfile_test.cpp)
target_link_libraries(logfile_test PRIVATE MyObjects)

add_executable(logger_test logger_test.cpp)
target_link_libraries(logger_test PRIVATE MyObjects)

add_executable(asynclogging_test asynclogging_test.cpp)
target_link_libraries(asynclogging_test PRIVATE MyObjects)

add_executable(inet_address_unittest inet_address_unittest.cpp)
target_link_libraries(inet_address_unittest PRIVATE MyObjects)
target_link_libraries(inet_address_unittest PRIVATE gtest gtest_main pthread)
add_test(NAME inet_address_unittest COMMAND inet_address_unittest)

add_executable(tcpserver_test tcpserver_test.cpp)
target_link_libraries(tcpserver_test PRIVATE MyObjects)

add_executable(buffer_unittest buffer_unittest.cpp)
target_link_libraries(buffer_unittest PRIVATE MyObjects)
target_link_libraries(buffer_unittest PRIVATE gtest gtest_main pthread)
add_test(NAME buffer_unittest COMMAND buffer_unittest)


add_executable(tcpconn_send_test tcpconn_send_test.cpp)
target_link_libraries(tcpconn_send_test PRIVATE MyObjects)

add_executable(tcpconn_send2_test tcpconn_send2_test.cpp)
target_link_libraries(tcpconn_send2_test PRIVATE MyObjects)

add_executable(tcpconn_send3_test tcpconn_send3_test.cpp)
target_link_libraries(tcpconn_send3_test PRIVATE MyObjects)

add_executable(http_parser_unittest http_parser_unittest.cpp)
target_link_libraries(http_parser_unittest PRIVATE MyObjects)
target_link_libraries(http_parser_unittest PRIVATE gtest gtest_main pthread)
add_test(NAME http_parser_unittest COMMAND http_parser_unittest)

add_executable(http_parser_with_tcp_server_test http_parser_with_tcp_server_test.cpp)
target_link_libraries(http_parser_with_tcp_server_test PRIVATE MyObjects)

add_executable(httpserver_demo_test httpserver_demo_test.cpp)
target_link_libraries(httpserver_demo_test PRIVATE MyObjects)

add_executable(config_test config_test.cpp)
target_link_libraries(config_test PRIVATE MyObjects)

add_executable(any_class_test any_class_test.cpp)
target_link_libraries(any_class_test PRIVATE MyObjects)

add_executable(http_server_unittest http_server_unittest.cpp)
target_link_libraries(http_server_unittest PRIVATE MyObjects)
target_link_libraries(http_server_unittest PRIVATE gtest gtest_main pthread)
add_test(NAME http_server_unittest COMMAND http_server_unittest)

add_executable(timewheel_test timewheel_test.cpp)
target_link_libraries(timewheel_test PRIVATE MyObjects)


add_custom_target(all_test_exec DEPENDS
    eventloop1_test
    eventloop2_test
    channel_epoller_test
    thread_test
    timer_test
    timer_self_cancel_test
    pending_functor_in_loop_test
    eventloop_thread_test 
    acceptor_test
    logfile_test
    logger_test
    asynclogging_test
    inet_address_unittest
    tcpserver_test
    buffer_unittest
    tcpconn_send_test
    tcpconn_send2_test
    tcpconn_send3_test
    http_parser_unittest
    http_parser_with_tcp_server_test
    httpserver_demo_test
    config_test
    any_class_test
    http_server_unittest
    timewheel_test
)
project(websocketpp)

include_directories(${Boost_INCLUDE_DIRS})

set(websocketpp_SRCS base64/base64.cpp
                     md5/md5.c
                     messages/data.cpp
                     network_utilities.cpp
                     processors/hybi_header.cpp
                     processors/hybi_util.cpp
                     sha1/sha1.cpp
                     uri.cpp )


file(GLOB websocketpp_root_HDRS  *.hpp)
file(GLOB websocketpp_http_HDRS  http/*.hpp)
file(GLOB websocketpp_logger_HDRS  logger/*.hpp)
file(GLOB websocketpp_messages_HDRS  messages/*.hpp)
file(GLOB websocketpp_processors_HDRS  processors/*.hpp)
file(GLOB websocketpp_rng_HDRS  rng/*.hpp)
file(GLOB websocketpp_roles_HDRS  roles/*.hpp)
file(GLOB websocketpp_sockets_HDRS  sockets/*.hpp)
file(GLOB websocketpp_utf8_validator_HDRS  utf8_validator/*.hpp)

set(websocketpp_base64_HDRS base64/base64.h)
set(websocketpp_sha1_HDRS sha1/sha1.h)
set(websocketpp_md5_HDRS md5/md5.h md5/md5.hpp)

set(websocketpp_HDRS base64/base64.h sha1/sha1.h md5/md5.hpp ${websocketpp_HDRS})

add_library(websocketpp STATIC ${websocketpp_SRCS})
set_target_properties(websocketpp PROPERTIES COMPILE_FLAGS -fPIC)
target_link_libraries(websocketpp ${Boost_LIBRARIES})

file(COPY ${websocketpp_root_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp)
file(COPY ${websocketpp_http_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/http)
file(COPY ${websocketpp_logger_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/logger)
file(COPY ${websocketpp_messages_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/messages)
file(COPY ${websocketpp_processors_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/processors)
file(COPY ${websocketpp_rng_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/rng)
file(COPY ${websocketpp_roles_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/roles)
file(COPY ${websocketpp_sockets_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/sockets)
file(COPY ${websocketpp_utf8_validator_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/utf8_validator)
file(COPY ${websocketpp_base64_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/base64)
file(COPY ${websocketpp_sha1_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/sha1)
file(COPY ${websocketpp_md5_HDRS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/websocketpp/md5)

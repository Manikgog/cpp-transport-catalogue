cmake_minimum_required(VERSION 3.31)
project(forbidden_domains)

set(CMAKE_CXX_STANDARD 20)

add_executable(forbidden_domains main.cpp 
domain.cpp 
geo.cpp 
json.cpp 
json_builder.cpp 
json_reader.cpp 
map_renderer.cpp 
request_handler.cpp 
svg.cpp 
transport_catalogue.cpp 
transport_router.cpp)

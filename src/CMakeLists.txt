cmake_minimum_required(VERSION 3.16)

project(Renewal VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(renewal_core
  build.cpp
  create_new.cpp
  generator.cpp
  manifest_parser.cpp
  utils.cpp
  validator.cpp
  workspace.cpp
)
target_include_directories(renewal_core PUBLIC ${CMAKE_SOURCE_DIR}/../include)

add_executable(renewal main.cpp)
target_link_libraries(renewal PRIVATE renewal_core)
target_include_directories(renewal PRIVATE ${CMAKE_SOURCE_DIR}/../include)

install(TARGETS renewal_core
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS renewal
  RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/../include/renewal
  DESTINATION include
)

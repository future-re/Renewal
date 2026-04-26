# Add library for core functionality
add_library(renewal_core
  build.cpp
  create_new.cpp
  generator.cpp
  manifest_parser.cpp
  toolchain.cpp
  utils.cpp
  validator.cpp
  workspace.cpp
)

# Specify include directories for the library
target_include_directories(renewal_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Add executable for the main application
add_executable(renewal main.cpp)

# Link the core library to the executable
target_link_libraries(renewal PRIVATE renewal_core)

target_link_libraries(renewal_core PUBLIC re2::re2)

install(TARGETS renewal_core
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS renewal
  RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/renewal
  DESTINATION include
)
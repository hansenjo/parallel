add_executable(ThreadTest ThreadTest.cxx)
target_link_libraries(ThreadTest Threads::Threads)
target_include_directories(ThreadTest PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(ThreadTest PRIVATE -O$<$<CONFIG:Debug>:0> -Wall)
set_target_properties(ThreadTest
  PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
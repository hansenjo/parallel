cmake_minimum_required (VERSION 3.8)   # Need >= 3.8 for C++17 support 

project(ppodd CXX)

set(PPODD ppodd)
set(PSRC DataFile.cxx Decoder.cxx Variable.cxx Detector.cxx
  DetectorTypeA.cxx DetectorTypeB.cxx Output.cxx Util.cxx Context.cxx)
string(REPLACE .cxx .h PHDR "${PSRC}")
list(APPEND PSRC ppodd.cxx)
list(APPEND PHDR Podd.h)

set(GENE generate)
set(GSRC generate.cxx)

add_executable(${PPODD} ${PSRC} ${PHDR})
add_executable(${GENE} ${GSRC})

target_compile_options(${PPODD}
  PRIVATE $<$<CONFIG:Debug>:-O0> -Wall
)
set_target_properties(${PPODD}
  PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(Boost_USE_MULTITHREADED TRUE)
find_package(Boost REQUIRED COMPONENTS iostreams)
target_link_libraries(${PPODD} Threads::Threads Boost::iostreams)

target_compile_options(${GENE}
  PRIVATE $<$<CONFIG:Debug>:-O0> -Wall
)
set_target_properties(${GENE}
  PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

install(TARGETS ${PPODD} ${GENE} DESTINATION bin)

add_subdirectory(Examples)


cmake_minimum_required (VERSION 3.8)   # Need >= 3.8 for C++17 support 

project(ppodd CXX)
option(PPODD_EVTORDER "Build event ordering code" OFF)

set(PPODD ppodd)
set(PSRC DataFile.cxx Decoder.cxx Variable.cxx Output.cxx Util.cxx Context.cxx
  Detector.cxx DetectorTypeA.cxx DetectorTypeB.cxx DetectorTypeC.cxx Database.cxx)
string(REPLACE .cxx .h PHDR "${PSRC}")
list(APPEND PHDR Podd.h)
add_executable(${PPODD} ${PPODD}.cxx ${PSRC} ${PHDR})

set(PPODDTBB ppodd-tbb)
add_executable(${PPODDTBB} ${PPODDTBB}.cxx ${PSRC} ${PHDR})

set(GENE generate)
set(GSRC generate.cxx)
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
if(PPODD_EVTORDER)
  target_compile_definitions(${PPODD} PRIVATE EVTORDER)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(Boost_USE_MULTITHREADED TRUE)
find_package(Boost REQUIRED COMPONENTS iostreams)
target_link_libraries(${PPODD} Threads::Threads Boost::iostreams)

find_package(TBB 2021 REQUIRED)
if(TBB_FOUND)
  message(STATUS "Found TBB ${TBB_VERSION} in ${TBB_DIR}")
endif()

target_compile_options(${PPODDTBB}
  PRIVATE $<$<CONFIG:Debug>:-O0> -Wall
  )
set_target_properties(${PPODDTBB}
  PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )
target_link_libraries(${PPODDTBB}
  Threads::Threads Boost::iostreams
  TBB::tbb TBB::tbbmalloc TBB::tbbmalloc_proxy
  )
target_compile_definitions(${PPODDTBB} PRIVATE PPODD_TBB)

target_compile_options(${GENE}
  PRIVATE $<$<CONFIG:Debug>:-O0> -Wall
)
set_target_properties(${GENE}
  PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

install(TARGETS ${PPODD} ${PPODDTBB} ${GENE} DESTINATION bin)

add_subdirectory(Examples)


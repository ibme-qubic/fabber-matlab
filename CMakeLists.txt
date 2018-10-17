cmake_minimum_required(VERSION 2.7)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(APPLE)
  # Uncomment to force use of libstdc++ on Mac as
  # FSL uses this by default. Not required if building FSL from
  # Cmake sources
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libstdc++")
  # Get rid of CMake warning
  set(CMAKE_MACOSX_RPATH ON)
endif(APPLE)

if (UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long")
endif(UNIX)

find_package(Matlab COMPONENTS MX_LIBRARY REQUIRED)

set(SRC fabber_wrapper.cpp)

# Find FSL dependencies

find_path(FSL_INCLUDE_DIR utils/tracer_plus.h
          HINTS $ENV{FSLDIR}/include REQUIRED)
find_path(FSL_EXTRAS_INCLUDE_DIR cprob/cprob.h
          HINTS $ENV{FSLDIR}/extras/include REQUIRED)
find_path(NEWMAT_INCLUDE_DIR newmat.h
          HINTS $ENV{FSLDIR}/extras/include/newmat REQUIRED)
find_library(ZNZ_LIBRARY NAMES znz libznz
             HINTS $ENV{FSLDIR}/lib REQUIRED )
find_library(MISCMATHS_LIBRARY NAMES miscmaths libmiscmaths
             HINTS $ENV{FSLDIR}/lib REQUIRED )
find_library(FSLIO_LIBRARY NAMES fslio libfslio
             HINTS $ENV{FSLDIR}/lib REQUIRED )
find_library(NIFTIIO_LIBRARY NAMES niftiio libniftiio
             HINTS $ENV{FSLDIR}/lib REQUIRED )
find_library(NEWMAT_LIBRARY NAMES newmat libnewmat
             HINTS $ENV{FSLDIR}/extras/lib REQUIRED )
find_library(FABBER_LIBRARY NAMES fabbercore libfabbercore
             HINTS $ENV{FSLDIR}/lib REQUIRED )

find_package(Boost)
if(NOT Boost_FOUND)
    find_path(Boost_INCLUDE_DIRS boost/shared_ptr.hpp
              HINTS $ENV{FSLDIR}/extras/include/boost REQUIRED)
endif(NOT Boost_FOUND)

if(NOT WIN32)
  find_package(ZLIB)
endif(NOT WIN32)
if (NOT ZLIB_FOUND)
  find_library(ZLIB_LIBRARY NAMES z libz zlib zlibd
               HINTS $ENV{FSLDIR}/lib REQUIRED NO_DEFAULT_PATH)
endif(NOT ZLIB_FOUND)

Message("-- FSL headers in ${FSL_INCLUDE_DIR} ${FSL_EXTRAS_INCLUDE_DIR} ${NEWMAT_INCLUDE_DIR}")
Message("-- Using fslio: ${FSLIO_LIBRARY}")
Message("-- Using miscmaths: ${MISCMATHS_LIBRARY}")
Message("-- Using niftiio: ${NIFTIIO_LIBRARY}")
Message("-- Using libznz: ${ZNZ_LIBRARY}")
Message("-- Using zlib: ${ZLIB_LIBRARY}")
Message("-- Using newmat: ${NEWMAT_LIBRARY}")
Message("-- Using Boost: ${Boost_INCLUDE_DIRS}")
Message("-- Using Matlab: ${Matlab_LIBRARIES}")

include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR} ${NEWMAT_INCLUDE_DIR} ${FSL_INCLUDE_DIR} ${FSL_EXTRAS_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

set(LIBS ${Matlab_LIBRARIES}
	 ${FABBER_LIBRARY}
	 ${FSLIO_LIBRARY}
         ${MISCMATHS_LIBRARY}
         ${NIFTIIO_LIBRARY}
         ${NEWMAT_LIBRARY} ${ZNZ_LIBRARY} ${ZLIB_LIBRARY})

if (UNIX)
  set(LIBS ${LIBS} dl)
endif(UNIX)

# Versioning information

# the commit's SHA1, and whether the building workspace was dirty or not
execute_process(COMMAND
  git describe --dirty
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_SHA1
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

# the date of the commit
execute_process(COMMAND
  git log -1 --format=%ad --date=local
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_DATE
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

if (NOT GIT_SHA1)
  set(GIT_SHA1 unknown)
endif(NOT GIT_SHA1)

if (NOT GIT_DATE)
  set(GIT_DATE unknown)
endif(NOT GIT_DATE)

add_definitions(-DGIT_SHA1="${GIT_SHA1}" -DGIT_DATE="${GIT_DATE}" -Dfabbercore_shared_EXPORTS)

# Main Targets

matlab_add_mex(NAME fabber_wrapper SRC ${SRC} LINK_TO ${LIBS})



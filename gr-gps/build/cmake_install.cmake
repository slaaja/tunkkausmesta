# Install script for directory: /Users/samu/koodit/gr/gr-gps

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/opt/local/Library/Frameworks/Python.framework/Versions/2.7")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/gps" TYPE FILE FILES "/Users/samu/koodit/gr/gr-gps/cmake/Modules/gpsConfig.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/samu/koodit/gr/gr-gps/build/include/gps/cmake_install.cmake")
  include("/Users/samu/koodit/gr/gr-gps/build/lib/cmake_install.cmake")
  include("/Users/samu/koodit/gr/gr-gps/build/swig/cmake_install.cmake")
  include("/Users/samu/koodit/gr/gr-gps/build/python/cmake_install.cmake")
  include("/Users/samu/koodit/gr/gr-gps/build/grc/cmake_install.cmake")
  include("/Users/samu/koodit/gr/gr-gps/build/apps/cmake_install.cmake")
  include("/Users/samu/koodit/gr/gr-gps/build/docs/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

file(WRITE "/Users/samu/koodit/gr/gr-gps/build/${CMAKE_INSTALL_MANIFEST}" "")
foreach(file ${CMAKE_INSTALL_MANIFEST_FILES})
  file(APPEND "/Users/samu/koodit/gr/gr-gps/build/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
endforeach()

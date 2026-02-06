# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/_deps/picotool-src"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/_deps/picotool-build"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/_deps"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/picotool/tmp"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/picotool/src/picotoolBuild-stamp"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/picotool/src"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/picotool/src/picotoolBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/picotool/src/picotoolBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/picotool/src/picotoolBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()

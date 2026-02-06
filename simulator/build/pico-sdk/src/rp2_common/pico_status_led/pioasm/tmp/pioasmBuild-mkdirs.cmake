# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Yogaratchagan/backup/projects/pico/pico-sdk/tools/pioasm"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/pioasm"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/pioasm-install"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/pico-sdk/src/rp2_common/pico_status_led/pioasm/tmp"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/pico-sdk/src/rp2_common/pico_status_led/pioasm/src/pioasmBuild-stamp"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/pico-sdk/src/rp2_common/pico_status_led/pioasm/src"
  "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/pico-sdk/src/rp2_common/pico_status_led/pioasm/src/pioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/pico-sdk/src/rp2_common/pico_status_led/pioasm/src/pioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Yogaratchagan/backup/projects/pico/health_monitor/simulator/build/pico-sdk/src/rp2_common/pico_status_led/pioasm/src/pioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()

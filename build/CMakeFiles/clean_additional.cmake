# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/ePaperPublisher_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/ePaperPublisher_autogen.dir/ParseCache.txt"
  "ePaperPublisher_autogen"
  )
endif()

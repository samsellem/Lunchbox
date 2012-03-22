
# Copyright (c) 2012 Stefan Eilemann <eile@eyescale.ch>

update_file(${CMAKE_CURRENT_SOURCE_DIR}/version.in.h
  ${OUTPUT_INCLUDE_DIR}/lunchbox/version.h)

set(LUNCHBOX_DEFINES)

if(LUNCHBOX_USE_BOOST_SERIALIZATION)
  list(APPEND LUNCHBOX_DEFINES LUNCHBOX_USE_BOOST_SERIALIZATION)
endif()

if(LUNCHBOX_OPENMP_USED)
  list(APPEND LUNCHBOX_DEFINES LUNCHBOX_USE_OPENMP)
endif(LUNCHBOX_OPENMP_USED)

if(HWLOC_FOUND)
  list(APPEND LUNCHBOX_DEFINES LUNCHBOX_USE_HWLOC)
endif(HWLOC_FOUND)

if(WIN32)
  list(APPEND LUNCHBOX_DEFINES WIN32 WIN32_API WIN32_LEAN_AND_MEAN)
  set(ARCH Win32)
endif(WIN32)

if(APPLE)
  list(APPEND LUNCHBOX_DEFINES Darwin)
  set(ARCH Darwin)
endif(APPLE)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  list(APPEND LUNCHBOX_DEFINES Linux)
  set(ARCH Linux)
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

set(DEFINES_FILE ${OUTPUT_INCLUDE_DIR}/lunchbox/defines${ARCH}.h)

file(WRITE ${DEFINES_FILE}
  "#ifndef LUNCHBOX_DEFINES_${ARCH}_H\n"
  "#define LUNCHBOX_DEFINES_${ARCH}_H\n\n"
  )

foreach(DEF ${LUNCHBOX_DEFINES})
  file(APPEND ${DEFINES_FILE}
    "#ifndef ${DEF}\n"
    "#  define ${DEF}\n"
    "#endif\n"
    )
endforeach(DEF ${LUNCHBOX_DEFINES})

file(APPEND ${DEFINES_FILE}
  "\n#endif /* LUNCHBOX_DEFINES_${ARCH}_H */\n"
  )
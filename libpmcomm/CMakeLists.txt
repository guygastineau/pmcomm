cmake_minimum_required(VERSION 2.6)
project(libpmcomm)

set(CMAKE_C_FLAGS "-Wall")

include_directories ("${PROJECT_SOURCE_DIR}/include")
add_library(pmcomm SHARED src/libpmcomm.c src/displayformat.c src/programformat.c src/pmconnection.c src/periodicdata.c src/profiledata.c src/efficiencydata.c)

set_target_properties(pmcomm PROPERTIES DEFINE_SYMBOL "BUILDING_LIBPMCOMM")

if(WIN32)
  set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-stdcall-alias")
  target_link_libraries(pmcomm ws2_32)
endif(WIN32)

if(UNIX)
  install(TARGETS pmcomm RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
endif(UNIX)
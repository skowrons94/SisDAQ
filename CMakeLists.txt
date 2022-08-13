cmake_minimum_required(VERSION 3.0)
project(SisDAQ VERSION 1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_INSTALL_DIR /opt/SisDAQ)

set(UI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ui)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${UI_DIR})

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(ROOT REQUIRED COMPONENTS RIO Net)
find_package(Boost COMPONENTS thread serialization REQUIRED)


include_directories(
  ${HEADER_DIR}
  ${ROOT_INCLUDE_DIR}
)

add_executable(SisDAQ
  ${SRC_DIR}/main.cc
  ${SRC_DIR}/SISInterface.cc
  ${SRC_DIR}/SISEthernetInterface.cc
  ${SRC_DIR}/SISSettingsBuilder.cc
  ${SRC_DIR}/SISSettings.cc
  ${SRC_DIR}/ReadoutUnit.cc
)

target_link_libraries(SisDAQ PRIVATE ${ROOT_LIBRARIES} ${Boost_LIBRARIES} Qt5::Widgets jsoncpp)

install(TARGETS SisDAQ
  DESTINATION ${CMAKE_INSTALL_DIR})

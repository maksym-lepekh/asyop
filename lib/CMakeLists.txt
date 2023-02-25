cmake_minimum_required(VERSION 3.24)

find_package(Threads REQUIRED)
include(FindPkgConfig)
pkg_check_modules(ASIO IMPORTED_TARGET asio)


# main library
add_library(asyop SHARED src/executor.cpp)
target_include_directories(asyop PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(asyop PUBLIC Threads::Threads)
target_compile_features(asyop PUBLIC cxx_std_17)
set_target_properties(asyop PROPERTIES
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})


# ASIO integration
if (ASIO_FOUND)
    add_library(asyop-asio SHARED src_asio/evloop_asio.cpp)
    target_include_directories(asyop-asio PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include_asio>
        $<INSTALL_INTERFACE:include_asio>)
    target_link_libraries(asyop-asio PUBLIC asyop PkgConfig::ASIO)
    set_target_properties(asyop-asio PROPERTIES EXPORT_NAME asio)
    set_target_properties(asyop-asio PROPERTIES
        VERSION ${CMAKE_PROJECT_VERSION}
        SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})
endif()


if (NOT PROJECT_IS_TOP_LEVEL)
    # just create imported target names
    add_library(asyop::asyop ALIAS asyop)
    if (ASIO_FOUND)
        add_library(asyop::asio ALIAS asyop-asio)
    endif()
else()
    # install
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    set(ASYOP_CMAKE_CONFIG_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/asyop")

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/asyop-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/asyop-config.cmake
        INSTALL_DESTINATION ${ASYOP_CMAKE_CONFIG_DESTINATION})

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/asyop-config-version.cmake"
        COMPATIBILITY SameMajorVersion)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/asyop-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/asyop-config-version.cmake"
        DESTINATION ${ASYOP_CMAKE_CONFIG_DESTINATION})

    install(TARGETS asyop EXPORT asyop DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY include/asy DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    if (ASIO_FOUND)
        install(TARGETS asyop-asio EXPORT asyop DESTINATION ${CMAKE_INSTALL_LIBDIR})
        install(DIRECTORY include_asio/asy DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif()

    install(EXPORT asyop NAMESPACE asyop:: DESTINATION ${ASYOP_CMAKE_CONFIG_DESTINATION})
endif()

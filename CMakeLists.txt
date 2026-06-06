cmake_minimum_required(VERSION 3.16)

# Ensure dependencies (vcpkg) and the toolchain BEFORE project(), so opening
# the project in CLion installs everything automatically. See the file header.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AutoBootstrap.cmake)

project(Chronexa VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(Chronexa
    src/main.cpp

    src/core/AppEnvironment.hpp
    src/core/AppEnvironment.cpp
    src/core/AppInitializer.hpp
    src/core/AppInitializer.cpp
    src/core/FileLogger.hpp
    src/core/FileLogger.cpp

    src/domain/activity/Activity.hpp
    src/domain/activity/Activity.cpp

    src/application/activity/ActivityService.hpp
    src/application/activity/ActivityService.cpp

    src/infrastructure/activity/IUserActivityProvider.hpp
    src/infrastructure/activity/WindowsActivityProvider.hpp
    src/infrastructure/activity/WindowsActivityProvider.cpp

    src/ui/activity/UserActivityController.hpp
    src/ui/activity/UserActivityController.cpp
    src/ui/activity/UserActivityModel.hpp
    src/ui/activity/UserActivityModel.cpp
)

target_include_directories(Chronexa PRIVATE src)

target_compile_definitions(Chronexa PRIVATE QT_MESSAGELOGCONTEXT)

target_link_libraries(Chronexa PRIVATE Qt6::Quick)

file(GLOB_RECURSE QML_FILES CONFIGURE_DEPENDS src/ui/qml/*.qml)

foreach(qml_file IN LISTS QML_FILES)
    file(RELATIVE_PATH alias ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qml ${qml_file})
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${alias})
endforeach()

qt_add_qml_module(Chronexa
    URI Chronexa
    QML_FILES ${QML_FILES}
)

set_target_properties(Chronexa PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
)

# Make the app runnable from any launcher (CLion, Visual Studio, double-click)
# without setting QT_PLUGIN_PATH/PATH by hand. vcpkg copies the dependent DLLs
# next to the .exe automatically, but NOT the Qt plugins -- the missing
# platform plugin is what causes "no Qt platform plugin could be initialized".
#
# windeployqt is incompatible with vcpkg's split debug/release layout, so we
# instead generate a qt.conf beside the .exe that points the Qt plugin and QML
# paths at the vcpkg install tree (debug vs release picked per config). The
# project's own QML module is embedded in the binary via resources, so only
# Qt's modules need to be found on disk.
if(WIN32 AND VCPKG_INSTALLED_DIR)
    set(_chronexa_vcpkg_root "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

    # 1) qt.conf tells Qt where to find the platform plugin and QML modules
    #    inside the vcpkg tree (debug vs release picked per config), so they
    #    don't have to be copied next to the .exe.
    file(GENERATE
        OUTPUT "$<TARGET_FILE_DIR:Chronexa>/qt.conf"
        CONTENT "[Paths]
Prefix = $<IF:$<CONFIG:Debug>,${_chronexa_vcpkg_root}/debug,${_chronexa_vcpkg_root}>
Plugins = Qt6/plugins
Qml2Imports = Qt6/qml
")

    # 2) The QML plugins load Qt module DLLs (Qt6Quick, Qt6QuickControls2, ...)
    #    at runtime, but vcpkg only deploys the .exe's direct imports next to
    #    it -- so Windows can't resolve those. Copy the Qt module DLLs beside
    #    the .exe (per config) to close that gap. copy_if_different keeps
    #    incremental builds cheap.
    file(GLOB _chronexa_qt_dlls_debug   "${_chronexa_vcpkg_root}/debug/bin/Qt6*.dll")
    file(GLOB _chronexa_qt_dlls_release "${_chronexa_vcpkg_root}/bin/Qt6*.dll")
    add_custom_command(TARGET Chronexa POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${_chronexa_qt_dlls_debug},${_chronexa_qt_dlls_release}>"
            "$<TARGET_FILE_DIR:Chronexa>"
        COMMAND_EXPAND_LISTS
        COMMENT "Copying Qt module DLLs next to Chronexa.exe"
        VERBATIM
    )
endif()

include(GNUInstallDirs)
install(TARGETS Chronexa
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

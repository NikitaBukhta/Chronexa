function(chronexa_add_test name)
    add_executable(${name} ${name}.cpp ${ARGN})
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${name} PRIVATE Qt6::Test Qt6::Core)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

chronexa_add_test(tst_CategoryRules
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
)

chronexa_add_test(tst_SqliteActivityRepository
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/SqliteActivityRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
)
target_link_libraries(tst_SqliteActivityRepository PRIVATE Qt6::Sql)

chronexa_add_test(tst_ActivityQueryService
    ${CMAKE_SOURCE_DIR}/src/application/activity/ActivityQueryService.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/ActivityStats.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
)

chronexa_add_test(tst_CategoryRulesModel
    ${CMAKE_SOURCE_DIR}/src/ui/activity/CategoryRulesModel.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppEnvironment.cpp
    ${CMAKE_SOURCE_DIR}/src/core/FileLogger.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)

chronexa_add_test(tst_PollingActivityProvider
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/PollingActivityProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
)

# ActivityService's default constructor builds the Windows probe, so it links
# in; the tests themselves only use the constructor that takes a provider.
chronexa_add_test(tst_ActivityService
    ${CMAKE_SOURCE_DIR}/src/application/activity/ActivityService.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/PollingActivityProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/WindowsForegroundProbe.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)
target_link_libraries(tst_ActivityService PRIVATE user32 version)

# End-to-end: drives the real app and takes the foreground, so it is labelled
# and left out of `bootstrap.py test`; run it with `bootstrap.py e2e`.
find_program(CHRONEXA_PYTHON NAMES python
    PATHS ${CMAKE_SOURCE_DIR}/venv/Scripts NO_DEFAULT_PATH)
if(NOT CHRONEXA_PYTHON)
    find_package(Python3 COMPONENTS Interpreter)
    set(CHRONEXA_PYTHON ${Python3_EXECUTABLE})
endif()
if(CHRONEXA_PYTHON)
    add_test(NAME e2e_categories
        COMMAND ${CHRONEXA_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/e2e/e2e_categories.py
                $<TARGET_FILE:Chronexa>)
    set_tests_properties(e2e_categories PROPERTIES
        LABELS e2e RUN_SERIAL TRUE TIMEOUT 180)
endif()

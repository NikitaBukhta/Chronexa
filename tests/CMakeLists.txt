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

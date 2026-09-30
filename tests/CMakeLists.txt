function(chronexa_add_test name)
    add_executable(${name} ${name}.cpp ${ARGN})
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${name} PRIVATE Qt6::Test Qt6::Core)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

chronexa_add_test(tst_Activity
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
)

chronexa_add_test(tst_CategoryRules
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
)

chronexa_add_test(tst_PrivacyRules
    ${CMAKE_SOURCE_DIR}/src/domain/activity/PrivacyRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
)

chronexa_add_test(tst_SqliteActivityRepository
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/SqliteActivityRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
)
target_link_libraries(tst_SqliteActivityRepository PRIVATE Qt6::Sql)

chronexa_add_test(tst_ActivityQueryService
    ${CMAKE_SOURCE_DIR}/src/application/activity/ActivityQueryService.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/ActivityStats.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
)

chronexa_add_test(tst_CategoryRulesModel
    ${CMAKE_SOURCE_DIR}/src/ui/activity/CategoryRulesModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/ActivityFormat.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/PrivacyRules.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/DailyGoals.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/GoalDigest.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppEnvironment.cpp
    ${CMAKE_SOURCE_DIR}/src/core/FileLogger.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)

chronexa_add_test(tst_PrivacyRulesModel
    ${CMAKE_SOURCE_DIR}/src/ui/activity/PrivacyRulesModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/ActivityFormat.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/DailyGoals.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/GoalDigest.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppEnvironment.cpp
    ${CMAKE_SOURCE_DIR}/src/core/FileLogger.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/PrivacyRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)

chronexa_add_test(tst_PollingActivityProvider
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/PollingActivityProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/PrivacyRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
)

chronexa_add_test(tst_DailyGoals
    ${CMAKE_SOURCE_DIR}/src/domain/activity/DailyGoals.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/GoalDigest.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)

chronexa_add_test(tst_GoalService
    ${CMAKE_SOURCE_DIR}/src/application/activity/GoalService.cpp
    ${CMAKE_SOURCE_DIR}/src/application/activity/ActivityQueryService.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/DailyGoals.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/GoalDigest.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/ActivityStats.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
)

chronexa_add_test(tst_GoalDigest
    ${CMAKE_SOURCE_DIR}/src/domain/activity/GoalDigest.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/DailyGoals.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)

chronexa_add_test(tst_GoalDigestService
    ${CMAKE_SOURCE_DIR}/src/application/activity/GoalDigestService.cpp
    ${CMAKE_SOURCE_DIR}/src/application/activity/ActivityQueryService.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/GoalDigest.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/DailyGoals.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/ActivityStats.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
)

chronexa_add_test(tst_GoalsModel
    ${CMAKE_SOURCE_DIR}/src/ui/activity/GoalsModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/CategoryRulesModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/ActivityFormat.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppEnvironment.cpp
    ${CMAKE_SOURCE_DIR}/src/core/FileLogger.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/DailyGoals.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/GoalDigest.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/PrivacyRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)

chronexa_add_test(tst_GoalController
    ${CMAKE_SOURCE_DIR}/src/ui/activity/GoalController.cpp
    ${CMAKE_SOURCE_DIR}/src/application/activity/GoalDigestService.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/GoalProgressModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/ActivityFormat.cpp
    ${CMAKE_SOURCE_DIR}/src/application/activity/GoalService.cpp
    ${CMAKE_SOURCE_DIR}/src/application/activity/ActivityQueryService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppEnvironment.cpp
    ${CMAKE_SOURCE_DIR}/src/core/FileLogger.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/DailyGoals.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/GoalDigest.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/ActivityStats.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/PrivacyRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)

# ActivityService's default constructor builds the Windows probe, so it links
# in; the tests themselves only use the constructor that takes a provider.
chronexa_add_test(tst_ActivityService
    ${CMAKE_SOURCE_DIR}/src/application/activity/ActivityService.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/PollingActivityProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/WindowsForegroundProbe.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/PrivacyRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)
target_link_libraries(tst_ActivityService PRIVATE user32 version)

# The log's session editor: the real QML over the real stack, headless. The
# QML comes from the module directory the Chronexa target writes, so the app
# is built first.
chronexa_add_test(tst_SessionEditing
    ${CMAKE_SOURCE_DIR}/src/ui/activity/ActivityQueryController.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/UserActivityController.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/UserActivityModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/AppTotalsModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/TimeBucketModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/ActivityFormat.cpp
    ${CMAKE_SOURCE_DIR}/src/application/activity/ActivityQueryService.cpp
    ${CMAKE_SOURCE_DIR}/src/application/activity/ActivityService.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/SqliteActivityRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/PollingActivityProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure/activity/WindowsForegroundProbe.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/DailyGoals.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/GoalDigest.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppEnvironment.cpp
    ${CMAKE_SOURCE_DIR}/src/core/FileLogger.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/ActivityStats.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/PrivacyRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)
target_link_libraries(tst_SessionEditing PRIVATE
    Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Sql user32 version)
target_compile_definitions(tst_SessionEditing PRIVATE
    CHRONEXA_QML_IMPORT_DIR="${CMAKE_BINARY_DIR}"
    CHRONEXA_QML_SOURCE_DIR="${CMAKE_SOURCE_DIR}/src/ui/qml")
add_dependencies(tst_SessionEditing Chronexa)

# The goal editor and the day page's goal card, headless like the above.
chronexa_add_test(tst_GoalsUi
    ${CMAKE_SOURCE_DIR}/src/ui/activity/GoalsModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/GoalController.cpp
    ${CMAKE_SOURCE_DIR}/src/application/activity/GoalDigestService.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/GoalProgressModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/CategoryRulesModel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/activity/ActivityFormat.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/settings/SettingsController.cpp
    ${CMAKE_SOURCE_DIR}/src/application/activity/GoalService.cpp
    ${CMAKE_SOURCE_DIR}/src/application/activity/ActivityQueryService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppEnvironment.cpp
    ${CMAKE_SOURCE_DIR}/src/core/FileLogger.cpp
    ${CMAKE_SOURCE_DIR}/src/core/TranslationManager.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/DailyGoals.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/GoalDigest.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/Activity.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/ActivityStats.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/CategoryRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/PrivacyRules.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/WindowMatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/activity/TrackingSchedule.cpp
)
target_link_libraries(tst_GoalsUi PRIVATE
    Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)
target_compile_definitions(tst_GoalsUi PRIVATE
    CHRONEXA_QML_IMPORT_DIR="${CMAKE_BINARY_DIR}"
    CHRONEXA_QML_SOURCE_DIR="${CMAKE_SOURCE_DIR}/src/ui/qml")
add_dependencies(tst_GoalsUi Chronexa)

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
    add_test(NAME e2e_privacy
        COMMAND ${CHRONEXA_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/e2e/e2e_privacy.py
                $<TARGET_FILE:Chronexa>)
    set_tests_properties(e2e_privacy PROPERTIES
        LABELS e2e RUN_SERIAL TRUE TIMEOUT 180)
    add_test(NAME e2e_goals
        COMMAND ${CHRONEXA_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/e2e/e2e_goals.py
                $<TARGET_FILE:Chronexa>)
    set_tests_properties(e2e_goals PROPERTIES
        LABELS e2e RUN_SERIAL TRUE TIMEOUT 360)
    add_test(NAME e2e_digest
        COMMAND ${CHRONEXA_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/e2e/e2e_digest.py
                $<TARGET_FILE:Chronexa>)
    set_tests_properties(e2e_digest PROPERTIES
        LABELS e2e RUN_SERIAL TRUE TIMEOUT 180)
endif()

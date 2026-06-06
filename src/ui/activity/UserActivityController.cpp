#include "UserActivityController.hpp"
#include "application/activity/ActivityService.hpp"

#include <QLoggingCategory>

namespace {

Q_LOGGING_CATEGORY(lcController, "chronexa.activity.controller")

}

namespace chronexa::activity {

UserActivityController::UserActivityController(ActivityService *service, QObject *parent)
    : QObject(parent), _service(service) {}

void UserActivityController::clearActivities() {
  qCInfo(lcController) << "clearActivities() invoked";
  _service->requestClear();
}

} // namespace chronexa::activity

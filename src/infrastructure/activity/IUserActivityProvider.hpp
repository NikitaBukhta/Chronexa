#pragma once

#include <QList>
#include <QString>

#include <functional>
#include <optional>

#include "domain/activity/Activity.hpp"

namespace chronexa::activity {

enum class Privacy;

class IUserActivityProvider {
public:
  // Decides which title changes within one application start a new session:
  // a change of key does, anything else only updates the title. Called on the
  // polling path, so it must be cheap and must not share state with the
  // caller's thread.
  using SessionKey =
      std::function<QString(const QString &appName, const QString &title)>;

  // Applied to every sample before it becomes part of a session: an excluded
  // window counts as no window, a hidden title as an empty one, so neither
  // the session key nor currentSession() ever sees what it withholds. Same
  // threading rules as SessionKey.
  using PrivacyFilter =
      std::function<Privacy(const QString &appName, const QString &title)>;

  virtual ~IUserActivityProvider() = default;

  virtual void setSessionKey(SessionKey key) = 0;
  virtual void setPrivacyFilter(PrivacyFilter filter) = 0;

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool isRunning() const = 0;
  virtual QList<Activity> drainEvents() = 0;
  virtual std::optional<Activity> currentSession() const = 0;
  virtual bool isIdle() const = 0;
};

} // namespace chronexa::activity

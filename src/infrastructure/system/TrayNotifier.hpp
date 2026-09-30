#pragma once

#include "INotifier.hpp"

#include <QObject>

#include <memory>

class QAction;
class QMenu;
class QSystemTrayIcon;

namespace chronexa::system {

// The icon in the notification area: shows messages as tray balloons (toasts
// on Windows 10+) and offers "open" and "quit". Needs a QApplication.
class TrayNotifier : public QObject, public INotifier {
  Q_OBJECT

public:
  explicit TrayNotifier(QObject *parent = nullptr);
  ~TrayNotifier() override;

  bool isSupported() const override;
  void notify(const QString &title, const QString &message) override;

  void retranslate();

signals:
  void openRequested();
  void quitRequested();

private:
  std::unique_ptr<QMenu> _menu;
  QSystemTrayIcon *_icon = nullptr;
  QAction *_openAction = nullptr;
  QAction *_quitAction = nullptr;
};

} // namespace chronexa::system

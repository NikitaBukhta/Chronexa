#include "TrayNotifier.hpp"

#include <QAction>
#include <QIcon>
#include <QLoggingCategory>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>

namespace {

Q_LOGGING_CATEGORY(lcTray, "chronexa.system.tray")

constexpr int kMessageMs = 10 * 1000;

// Drawn rather than shipped: the app has no image resources, and a tray icon
// without a pixmap is not shown at all.
QIcon trayIcon() {
  QIcon icon;
  for (const int size : {16, 20, 24, 32, 48, 64}) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0x2a, 0x78, 0xd6));
    painter.drawEllipse(QRectF(0.5, 0.5, size - 1.0, size - 1.0));

    const QPointF centre(size / 2.0, size / 2.0);
    QPen hand(Qt::white, qMax(1.5, size / 10.0), Qt::SolidLine, Qt::RoundCap);
    painter.setPen(hand);
    painter.drawLine(centre, centre + QPointF(0, -size * 0.30));
    painter.drawLine(centre, centre + QPointF(size * 0.22, size * 0.12));
    painter.end();

    icon.addPixmap(pixmap);
  }
  return icon;
}

} // namespace

namespace chronexa::system {

TrayNotifier::TrayNotifier(QObject *parent)
    : QObject(parent), _menu(std::make_unique<QMenu>()) {
  _openAction = _menu->addAction(QString());
  _menu->addSeparator();
  _quitAction = _menu->addAction(QString());
  connect(_openAction, &QAction::triggered, this, &TrayNotifier::openRequested);
  connect(_quitAction, &QAction::triggered, this, &TrayNotifier::quitRequested);

  _icon = new QSystemTrayIcon(trayIcon(), this);
  _icon->setContextMenu(_menu.get());
  connect(_icon, &QSystemTrayIcon::activated, this,
          [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger ||
                reason == QSystemTrayIcon::DoubleClick) {
              emit openRequested();
            }
          });
  connect(_icon, &QSystemTrayIcon::messageClicked, this,
          &TrayNotifier::openRequested);

  retranslate();

  if (!isSupported()) {
    qCWarning(lcTray) << "No system tray -- goal notifications are off";
    return;
  }
  _icon->show();
  qCInfo(lcTray) << "Tray icon shown";
}

TrayNotifier::~TrayNotifier() {
  // The icon holds a pointer to the menu: drop it first.
  if (_icon != nullptr) {
    _icon->hide();
    _icon->setContextMenu(nullptr);
  }
}

bool TrayNotifier::isSupported() const {
  return QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayNotifier::notify(const QString &title, const QString &message) {
  if (!isSupported()) {
    qCInfo(lcTray) << "Notification dropped, no tray:" << title;
    return;
  }
  qCInfo(lcTray) << "Notification:" << title;
  _icon->showMessage(title, message, QSystemTrayIcon::Information, kMessageMs);
}

void TrayNotifier::retranslate() {
  _icon->setToolTip(tr("Chronexa"));
  _openAction->setText(tr("Open Chronexa"));
  _quitAction->setText(tr("Quit"));
}

} // namespace chronexa::system

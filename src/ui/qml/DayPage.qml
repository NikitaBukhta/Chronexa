import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root

    required property var query
    required property var monthRange

    readonly property date selectedDay: query.selectedDay
    readonly property bool showingToday: sameDay(selectedDay, new Date())

    function sameDay(a, b) {
        return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth() && a.getDate() === b.getDate();
    }

    function sameMonth(a, b) {
        return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth();
    }

    function syncMonth() {
        const day = root.selectedDay;
        const first = new Date(day.getFullYear(), day.getMonth(), 1);
        const last = new Date(day.getFullYear(), day.getMonth() + 1, 0);
        if (!root.sameMonth(monthRange.rangeFrom, day))
            monthRange.setDayRange(first, last);
    }

    Component.onCompleted: {
        monthRange.granularity = "day";
        syncMonth();
    }

    onSelectedDayChanged: syncMonth()

    ScrollView {
        id: scroller

        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroller.availableWidth
            spacing: Theme.gap

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                Layout.topMargin: Theme.gapLoose
                spacing: Theme.gapTight

                ToolButtonBase {
                    glyph: "‹"
                    compact: true
                    onClicked: root.query.shiftRange(-1)
                }

                ToolButtonBase {
                    // query.selectedDay, not root.selectedDay: rangeChanged is
                    // re-emitted on a language switch, the local copy is not.
                    text: activityController.formatDayLabel(root.query.selectedDay)
                    glyph: "▾"
                    glyphTrailing: true
                    onClicked: dayCalendar.open()

                    CalendarPopup {
                        id: dayCalendar

                        y: parent.height + 4
                        selected: root.selectedDay
                        onPicked: function (picked) {
                            root.query.selectDay(picked);
                        }
                    }
                }

                ToolButtonBase {
                    glyph: "›"
                    compact: true
                    enabled: !root.showingToday
                    opacity: enabled ? 1 : 0.4
                    onClicked: root.query.shiftRange(1)
                }

                ToolButtonBase {
                    text: qsTr("Today")
                    visible: !root.showingToday
                    onClicked: root.query.applyPreset("today")
                }

                Item {
                    Layout.fillWidth: true
                }

                Text {
                    text: root.selectedDay.toLocaleDateString(Qt.locale(settingsController.resolvedLanguage), Locale.LongFormat)
                    color: Theme.inkFaint
                    font.pixelSize: Theme.fontLabel
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                title: Qt.locale(settingsController.resolvedLanguage).standaloneMonthName(root.monthRange.rangeFrom.getMonth()) + " " + root.monthRange.rangeFrom.getFullYear()
                subtitle: qsTr("%1 active of %2 days · %3 total").arg(root.monthRange.activeDayCount).arg(root.monthRange.dayCount).arg(activityController.formatDuration(root.monthRange.totalSeconds, true))

                headerContent: [
                    ToolButtonBase {
                        glyph: "‹"
                        compact: true
                        onClicked: root.monthRange.shiftRange(-1)
                    },
                    ToolButtonBase {
                        glyph: "›"
                        compact: true
                        enabled: root.monthRange.rangeTo <= new Date()
                        opacity: enabled ? 1 : 0.4
                        onClicked: root.monthRange.shiftRange(1)
                    }
                ]

                DayStrip {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    dayModel: root.monthRange.buckets
                    selectedDay: root.selectedDay
                    onDaySelected: function (value) {
                        root.query.selectDay(value);
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                spacing: Theme.gap

                Rectangle {
                    Layout.preferredWidth: 260
                    Layout.fillHeight: true
                    implicitHeight: hero.implicitHeight + Theme.pad * 2
                    color: Theme.surface
                    radius: Theme.radius
                    border.width: 1
                    border.color: Theme.border

                    ColumnLayout {
                        id: hero

                        anchors.fill: parent
                        anchors.margins: Theme.pad
                        spacing: 2

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Active time")
                            color: Theme.inkFaint
                            font.pixelSize: Theme.fontLabel
                        }

                        Text {
                            Layout.fillWidth: true
                            text: activityController.formatDuration(root.query.totalSeconds)
                            color: Theme.ink
                            font.pixelSize: Theme.fontHero
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.query.empty ? qsTr("Nothing recorded on this day") : qsTr("%1 → %2").arg(activityController.formatClock(root.query.firstActivity)).arg(activityController.formatClock(root.query.lastActivity))
                            color: Theme.inkMuted
                            font.pixelSize: Theme.fontLabel
                            elide: Text.ElideRight
                        }
                    }
                }

                StatTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: qsTr("Most used")
                    value: root.query.topAppName === "" ? "—" : activityController.formatDuration(root.query.topAppSeconds, true)
                    detail: root.query.topAppName === "" ? qsTr("No application") : root.query.topAppName
                    accent: root.query.topAppName === "" ? "transparent" : Theme.series(root.query.colorSlot(root.query.topAppName))
                }

                StatTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: qsTr("Longest stretch")
                    value: root.query.longestSessionSeconds === 0 ? "—" : activityController.formatDuration(root.query.longestSessionSeconds, true)
                    detail: root.query.longestSessionApp === "" ? qsTr("No session") : root.query.longestSessionApp
                }

                StatTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: qsTr("Applications")
                    value: root.query.appCount.toString()
                    detail: qsTr("%1 sessions").arg(root.query.sessionCount)
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                title: qsTr("Through the day")
                subtitle: qsTr("Each block is one session, placed where it happened")

                TimelineBand {
                    Layout.fillWidth: true
                    visible: !root.query.empty
                    sessionModel: root.query.sessions
                    dayStart: root.query.rangeFrom
                }

                EmptyState {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.gap
                    Layout.bottomMargin: Theme.gap
                    visible: root.query.empty
                    headline: qsTr("No activity on this day")
                    explanation: activityController.tracking ? qsTr("Nothing was recorded here. Pick another day above.") : qsTr("Tracking is paused, so nothing is being recorded.")
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                title: qsTr("Hour by hour")
                subtitle: qsTr("Active time per hour")

                BarChart {
                    Layout.fillWidth: true
                    visible: !root.query.empty
                    barModel: root.query.buckets
                    plotHeight: 170
                }

                EmptyState {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.gap
                    Layout.bottomMargin: Theme.gap
                    visible: root.query.empty
                    headline: qsTr("Nothing to plot")
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                visible: root.query.hasCategoryRules && !root.query.empty
                title: qsTr("By category")
                subtitle: qsTr("What the day went on")

                AppShareList {
                    Layout.fillWidth: true
                    totalsModel: root.query.categoryTotals
                    nameHeader: qsTr("Category")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                Layout.bottomMargin: Theme.gapLoose
                spacing: Theme.gap

                // Both cards take the row's height: the shorter one was
                // centred in it, leaving a gap above its title.
                Card {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    title: qsTr("By application")
                    subtitle: qsTr("The colours used above")

                    AppShareList {
                        Layout.fillWidth: true
                        totalsModel: root.query.appTotals
                        visibleRows: 9
                    }

                    EmptyState {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.gap
                        visible: root.query.empty
                        headline: qsTr("No applications")
                    }

                    // Keeps the list at the top of the stretched card.
                    Item {
                        Layout.fillHeight: true
                    }
                }

                Card {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    title: qsTr("Sessions")
                    subtitle: qsTr("%1 on this day, newest first").arg(root.query.sessionCount)

                    SessionTable {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 9 * Theme.rowHeight + 30
                        sessionModel: root.query.sessions
                    }
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root

    required property var query

    readonly property var presets: [
        { key: "today", label: qsTr("Today") },
        { key: "yesterday", label: qsTr("Yesterday") },
        { key: "last24h", label: qsTr("24 hours") },
        { key: "last7days", label: qsTr("7 days") },
        { key: "last30days", label: qsTr("30 days") },
        { key: "thismonth", label: qsTr("This month") },
        { key: "alltime", label: qsTr("All time") }
    ]

    readonly property var granularities: [
        { key: "hour", label: qsTr("Hourly") },
        { key: "day", label: qsTr("Daily") },
        { key: "week", label: qsTr("Weekly") },
        { key: "month", label: qsTr("Monthly") }
    ]

    ScrollView {
        id: scroller

        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroller.availableWidth
            spacing: Theme.gap

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                Layout.topMargin: Theme.gapLoose
                implicitHeight: filters.implicitHeight + Theme.pad * 2
                color: Theme.surface
                radius: Theme.radius
                border.width: 1
                border.color: Theme.border

                ColumnLayout {
                    id: filters

                    anchors.fill: parent
                    anchors.margins: Theme.pad
                    spacing: Theme.gap

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap

                        SegmentedControl {
                            options: root.presets
                            current: root.query.preset
                            onPicked: function (key) {
                                root.query.useAutoGranularity();
                                root.query.applyPreset(key);
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Text {
                            text: qsTr("Buckets")
                            color: Theme.inkFaint
                            font.pixelSize: Theme.fontLabel
                        }

                        SegmentedControl {
                            options: root.granularities
                            current: root.query.granularity
                            onPicked: function (key) {
                                root.query.granularity = key;
                            }
                        }

                        ToolButtonBase {
                            text: qsTr("Auto")
                            active: root.query.granularityAuto
                            compact: true
                            onClicked: root.query.useAutoGranularity()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.border
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gapTight

                        Text {
                            text: qsTr("From")
                            color: Theme.inkFaint
                            font.pixelSize: Theme.fontLabel
                        }

                        DateTimeField {
                            value: root.query.rangeFrom
                            onEdited: function (next) {
                                root.query.setRange(next, root.query.rangeTo);
                            }
                        }

                        Text {
                            Layout.leftMargin: Theme.gapTight
                            text: qsTr("to")
                            color: Theme.inkFaint
                            font.pixelSize: Theme.fontLabel
                        }

                        DateTimeField {
                            value: root.query.rangeTo
                            // The range end is exclusive, so "all of today"
                            // needs tomorrow to be selectable.
                            maximumDayOffset: 1
                            onEdited: function (next) {
                                root.query.setRange(root.query.rangeFrom, next);
                            }
                        }

                        ToolButtonBase {
                            Layout.leftMargin: Theme.gap
                            glyph: "‹"
                            compact: true
                            onClicked: root.query.shiftRange(-1)
                        }

                        ToolButtonBase {
                            glyph: "›"
                            compact: true
                            enabled: root.query.canShiftForward
                            opacity: enabled ? 1 : 0.4
                            onClicked: root.query.shiftRange(1)
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Text {
                            text: root.query.rangeLabel
                            color: Theme.inkMuted
                            font.pixelSize: Theme.fontLabel
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                spacing: Theme.gap

                Rectangle {
                    Layout.preferredWidth: 300
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
                            text: qsTr("Total active time")
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
                            text: root.query.rangeLabel
                            color: Theme.inkMuted
                            font.pixelSize: Theme.fontLabel
                            elide: Text.ElideRight
                        }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 3
                    rowSpacing: Theme.gap
                    columnSpacing: Theme.gap

                    StatTile {
                        Layout.fillWidth: true
                        label: qsTr("Per active day")
                        value: activityController.formatDuration(root.query.dailyAverageSeconds, true)
                        detail: qsTr("%1 of %2 days active").arg(root.query.activeDayCount).arg(root.query.dayCount)
                    }

                    StatTile {
                        Layout.fillWidth: true
                        label: qsTr("Most used")
                        value: root.query.topAppName === "" ? "—" : activityController.formatDuration(root.query.topAppSeconds, true)
                        detail: root.query.topAppName === "" ? qsTr("No application") : root.query.topAppName
                        accent: root.query.topAppName === "" ? "transparent" : Theme.series(root.query.colorSlot(root.query.topAppName))
                    }

                    StatTile {
                        Layout.fillWidth: true
                        label: qsTr("Longest stretch")
                        value: root.query.longestSessionSeconds === 0 ? "—" : activityController.formatDuration(root.query.longestSessionSeconds, true)
                        detail: root.query.longestSessionApp === "" ? qsTr("No session") : root.query.longestSessionApp
                    }

                    StatTile {
                        Layout.fillWidth: true
                        label: qsTr("Applications")
                        value: root.query.appCount.toString()
                        detail: qsTr("distinct in range")
                    }

                    StatTile {
                        Layout.fillWidth: true
                        label: qsTr("Sessions")
                        value: root.query.sessionCount.toString()
                        detail: root.query.sessionCount > 0 ? qsTr("avg %1").arg(activityController.formatDuration(Math.round(root.query.totalSeconds / root.query.sessionCount), true)) : qsTr("none recorded")
                    }

                    StatTile {
                        Layout.fillWidth: true
                        label: qsTr("First → last")
                        value: root.query.empty ? "—" : activityController.formatClock(root.query.firstActivity)
                        detail: root.query.empty ? qsTr("nothing in range") : qsTr("last at %1").arg(activityController.formatClock(root.query.lastActivity))
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                title: qsTr("Activity over the period")
                subtitle: root.query.granularity === "hour" ? qsTr("Active time per hour") : root.query.granularity === "day" ? qsTr("Active time per day") : root.query.granularity === "week" ? qsTr("Active time per week") : qsTr("Active time per month")

                BarChart {
                    Layout.fillWidth: true
                    visible: !root.query.empty
                    barModel: root.query.buckets
                    plotHeight: 210
                }

                EmptyState {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.gapLoose
                    Layout.bottomMargin: Theme.gapLoose
                    visible: root.query.empty
                    headline: qsTr("Nothing recorded in this period")
                    explanation: qsTr("Widen the range, or pick a preset above.")
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                Layout.bottomMargin: Theme.gapLoose
                title: qsTr("Where the time went")
                subtitle: qsTr("%1 applications · the colours used across the app").arg(root.query.appCount)

                AppShareList {
                    Layout.fillWidth: true
                    visible: !root.query.empty
                    totalsModel: root.query.appTotals
                    visibleRows: 12
                }

                EmptyState {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.gap
                    visible: root.query.empty
                    headline: qsTr("No applications in this period")
                }
            }
        }
    }
}

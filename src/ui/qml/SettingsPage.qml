import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root

    signal clearRequested

    readonly property var themeOptions: [
        { key: "dark", label: qsTr("Dark") },
        { key: "light", label: qsTr("Light") }
    ]

    ScrollView {
        id: scroller

        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroller.availableWidth
            spacing: Theme.gap

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                Layout.topMargin: Theme.gapLoose
                title: qsTr("Tracking")
                subtitle: activityController.tracking ? qsTr("Recording the active window") : activityController.trackingEnabled ? qsTr("On, but held back") : qsTr("Stopped — nothing is being recorded")

                SettingRow {
                    Layout.fillWidth: true
                    label: qsTr("Record activity")
                    explanation: qsTr("The master switch. Time spent while it is off is never stored.")

                    ToolButtonBase {
                        text: activityController.trackingEnabled ? qsTr("Stop") : qsTr("Start")
                        glyph: activityController.trackingEnabled ? "■" : "▶"
                        active: activityController.trackingEnabled
                        onClicked: activityController.toggleTracking()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.border
                }

                SettingRow {
                    Layout.fillWidth: true
                    label: qsTr("Only during set hours")
                    explanation: settingsController.scheduleSummary

                    AppSwitch {
                        checked: appSettings.scheduleEnabled
                        onRequested: function (value) {
                            appSettings.scheduleEnabled = value;
                        }
                    }
                }

                SettingRow {
                    Layout.fillWidth: true
                    Layout.topMargin: 2
                    enabledRow: appSettings.scheduleEnabled
                    label: qsTr("Hours")
                    explanation: appSettings.scheduleEndMinutes < appSettings.scheduleStartMinutes ? qsTr("Ends the next morning") : qsTr("Set the end before the start for an overnight shift")

                    MinutesField {
                        minutes: appSettings.scheduleStartMinutes
                        editable: appSettings.scheduleEnabled
                        onEdited: function (value) {
                            appSettings.scheduleStartMinutes = value;
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignVCenter
                        text: "–"
                        color: Theme.inkFaint
                        font.pixelSize: Theme.fontBody
                    }

                    MinutesField {
                        minutes: appSettings.scheduleEndMinutes
                        editable: appSettings.scheduleEnabled
                        onEdited: function (value) {
                            appSettings.scheduleEndMinutes = value;
                        }
                    }
                }

                SettingRow {
                    Layout.fillWidth: true
                    enabledRow: appSettings.scheduleEnabled
                    label: qsTr("Days")
                    explanation: qsTr("Days the schedule covers")

                    ToolButtonBase {
                        text: qsTr("Mon–Fri")
                        compact: true
                        enabled: appSettings.scheduleEnabled
                        onClicked: settingsController.setScheduleDaysPreset("workdays")
                    }

                    ToolButtonBase {
                        text: qsTr("All")
                        compact: true
                        enabled: appSettings.scheduleEnabled
                        onClicked: settingsController.setScheduleDaysPreset("everyday")
                    }
                }

                WeekdayPicker {
                    Layout.fillWidth: false
                    Layout.alignment: Qt.AlignRight
                    days: appSettings.scheduleDays
                    editable: appSettings.scheduleEnabled
                    onDayToggled: function (dayOfWeek) {
                        settingsController.toggleScheduleDay(dayOfWeek);
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                title: qsTr("Categories")
                subtitle: qsTr("The first matching rule decides, so put narrow rules (YouTube in a browser) above broad ones.")

                CategoryRuleList {
                    Layout.fillWidth: true
                    rulesModel: categoryRules
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                title: qsTr("Start-up")

                SettingRow {
                    Layout.fillWidth: true
                    enabledRow: settingsController.autoStartSupported
                    label: qsTr("Launch when I sign in")
                    explanation: settingsController.autoStartSupported ? qsTr("Chronexa starts with Windows, so a day is never half-recorded.") : qsTr("Not available on this platform.")

                    AppSwitch {
                        checked: settingsController.autoStart
                        enabled: settingsController.autoStartSupported
                        onRequested: function (value) {
                            settingsController.autoStart = value;
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                title: qsTr("Appearance")

                SettingRow {
                    Layout.fillWidth: true
                    label: qsTr("Theme")

                    SegmentedControl {
                        options: root.themeOptions
                        current: Theme.dark ? "dark" : "light"
                        onPicked: function (key) {
                            Theme.dark = key === "dark";
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.border
                }

                SettingRow {
                    Layout.fillWidth: true
                    label: qsTr("Language")
                    explanation: appSettings.language === "system" ? qsTr("Following the system, currently %1").arg(settingsController.resolvedLanguage) : qsTr("Dates and names follow the chosen language too.")

                    Repeater {
                        model: settingsController.languages

                        ToolButtonBase {
                            required property var modelData

                            text: modelData.label
                            compact: true
                            active: appSettings.language === modelData.code
                            onClicked: appSettings.language = modelData.code
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapLoose
                Layout.rightMargin: Theme.gapLoose
                Layout.bottomMargin: Theme.gapLoose
                title: qsTr("Data")

                SettingRow {
                    Layout.fillWidth: true
                    label: qsTr("Recorded history")
                    explanation: qsTr("Kept on this machine only, at %1").arg(appSettings.historyPath)

                    ToolButtonBase {
                        text: qsTr("Clear history…")
                        glyph: "⌫"
                        onClicked: root.clearRequested()
                    }
                }
            }
        }
    }
}

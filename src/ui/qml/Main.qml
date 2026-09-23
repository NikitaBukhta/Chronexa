import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root

    width: 1280
    height: 860
    minimumWidth: 1024
    minimumHeight: 640
    visible: true
    title: qsTr("Chronexa")
    color: Theme.plane

    readonly property var activePage: [dayQuery, periodQuery, logQuery, null][nav.currentIndex]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        NavRail {
            id: nav

            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            TopBar {
                Layout.fillWidth: true
                heading: nav.pages[nav.currentIndex].label
                subheading: root.activePage ? root.activePage.rangeLabel : settingsController.scheduleSummary
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: nav.currentIndex

                DayPage {
                    query: dayQuery
                    monthRange: monthQuery
                }

                PeriodPage {
                    query: periodQuery
                }

                LogPage {
                    query: logQuery
                }

                SettingsPage {
                    onClearRequested: clearDialog.open()
                }
            }
        }
    }

    Dialog {
        id: clearDialog

        anchors.centerIn: parent
        width: 400
        modal: true
        title: qsTr("Clear all history?")
        standardButtons: Dialog.Cancel | Dialog.Discard

        background: Rectangle {
            color: Theme.surfaceRaised
            radius: Theme.radius
            border.width: 1
            border.color: Theme.border
        }

        header: Text {
            padding: Theme.pad
            text: clearDialog.title
            color: Theme.ink
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }

        contentItem: Text {
            text: qsTr("Every recorded session is deleted. This cannot be undone.")
            color: Theme.inkMuted
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }

        onDiscarded: {
            activityController.clearActivities();
            clearDialog.close();
        }
    }

    Dialog {
        id: failureDialog

        property string message: ""

        anchors.centerIn: parent
        width: 400
        modal: true
        title: qsTr("That did not work")
        standardButtons: Dialog.Ok

        background: Rectangle {
            color: Theme.surfaceRaised
            radius: Theme.radius
            border.width: 1
            border.color: Theme.border
        }

        header: Text {
            padding: Theme.pad
            text: failureDialog.title
            color: Theme.ink
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }

        contentItem: Text {
            text: failureDialog.message
            color: Theme.inkMuted
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }

    Connections {
        target: settingsController

        function onFailed(message) {
            failureDialog.message = message;
            failureDialog.open();
        }
    }
}

import QtCore
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

    Settings {
        id: savedWindow

        category: "window"

        property bool saved: false
        property int x: 0
        property int y: 0
        property int width: 1280
        property int height: 860
        property bool maximized: false
    }

    function onAnyScreen(x, y) {
        const screens = Qt.application.screens;
        for (let i = 0; i < screens.length; ++i) {
            const s = screens[i];
            if (x >= s.virtualX && x < s.virtualX + s.width && y >= s.virtualY && y < s.virtualY + s.height)
                return true;
        }
        return false;
    }

    Component.onCompleted: {
        if (!savedWindow.saved)
            return;
        root.width = Math.max(root.minimumWidth, savedWindow.width);
        root.height = Math.max(root.minimumHeight, savedWindow.height);
        // A monitor unplugged since the last run must not strand the window
        // off-screen: the title bar has to land on a screen that still exists.
        if (onAnyScreen(savedWindow.x + 80, savedWindow.y + 10)) {
            root.x = savedWindow.x;
            root.y = savedWindow.y;
        }
        if (savedWindow.maximized)
            root.visibility = Window.Maximized;
    }

    onClosing: {
        savedWindow.maximized = root.visibility === Window.Maximized;
        if (root.visibility === Window.Windowed) {
            savedWindow.x = root.x;
            savedWindow.y = root.y;
            savedWindow.width = root.width;
            savedWindow.height = root.height;
        }
        savedWindow.saved = true;
        savedWindow.sync();
    }

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
                    id: settingsPage

                    onClearRequested: clearDialog.open()
                    onPrivacyApplyRequested: privacyDialog.open()
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
        id: privacyDialog

        anchors.centerIn: parent
        width: 400
        modal: true
        title: qsTr("Apply privacy rules to history?")
        standardButtons: Dialog.Cancel | Dialog.Apply

        background: Rectangle {
            color: Theme.surfaceRaised
            radius: Theme.radius
            border.width: 1
            border.color: Theme.border
        }

        header: Text {
            padding: Theme.pad
            text: privacyDialog.title
            color: Theme.ink
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }

        contentItem: Text {
            text: qsTr("Recorded sessions of excluded windows are deleted, and hidden titles are erased. This cannot be undone.")
            color: Theme.inkMuted
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }

        onApplied: {
            privacyDialog.close();
            const changed = activityController.applyPrivacyToHistory();
            if (changed < 0) {
                settingsPage.privacyStatus = "";
                failureDialog.message = qsTr("The history could not be changed. Nothing was deleted.");
                failureDialog.open();
            } else if (changed === 0) {
                settingsPage.privacyStatus = qsTr("History already follows the rules.");
            } else {
                settingsPage.privacyStatus = qsTr("%n session(s) updated.", "", changed);
            }
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

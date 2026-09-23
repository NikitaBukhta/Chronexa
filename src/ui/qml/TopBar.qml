import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string heading: ""
    property string subheading: ""

    readonly property bool live: activityController.tracking && activityController.hasCurrent && !activityController.idle
    readonly property color stateColor: !activityController.trackingEnabled ? Theme.inkFaint : !activityController.tracking || activityController.idle ? Theme.warning : Theme.good

    implicitHeight: 62
    color: Theme.surface

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.gapLoose
        anchors.rightMargin: Theme.gapLoose
        spacing: Theme.gap

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            Text {
                Layout.fillWidth: true
                text: root.heading
                color: Theme.ink
                font.pixelSize: Theme.fontTitle + 2
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                visible: root.subheading !== ""
                text: root.subheading
                color: Theme.inkFaint
                font.pixelSize: Theme.fontLabel
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.preferredWidth: Math.min(430, Math.max(210, implicitWidth))
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: liveRow.implicitWidth + Theme.gap * 2
            radius: Theme.radiusSmall
            color: Theme.surfaceSunken
            border.width: 1
            border.color: Theme.border

            RowLayout {
                id: liveRow

                anchors.fill: parent
                anchors.leftMargin: Theme.gap
                anchors.rightMargin: Theme.gap
                spacing: 10

                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: 8
                    implicitHeight: 8
                    radius: 4
                    color: root.stateColor

                    SequentialAnimation on opacity {
                        running: root.live
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.35; duration: 900; easing.type: Easing.InOutQuad }
                        NumberAnimation { to: 1.0; duration: 900; easing.type: Easing.InOutQuad }
                    }

                    onVisibleChanged: opacity = 1
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Text {
                        Layout.fillWidth: true
                        text: !activityController.trackingEnabled ? qsTr("Tracking stopped") : activityController.heldBySchedule ? qsTr("Outside the schedule") : activityController.idle ? qsTr("Idle") : root.live ? activityController.currentAppName : qsTr("Waiting for a window")
                        color: Theme.ink
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: !activityController.trackingEnabled ? qsTr("Nothing is being recorded") : activityController.heldBySchedule ? activityController.scheduleHoldText : activityController.idle ? qsTr("Idle time is not recorded") : activityController.currentTitle
                        color: Theme.inkFaint
                        font.pixelSize: Theme.fontMicro
                        elide: Text.ElideRight
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.live
                    text: activityController.currentSecondsText
                    color: Theme.inkMuted
                    font.pixelSize: Theme.fontBody
                }
            }
        }

        ToolButtonBase {
            Layout.alignment: Qt.AlignVCenter
            text: activityController.trackingEnabled ? qsTr("Stop") : qsTr("Start")
            glyph: activityController.trackingEnabled ? "■" : "▶"
            active: activityController.trackingEnabled
            onClicked: activityController.toggleTracking()
        }
    }
}

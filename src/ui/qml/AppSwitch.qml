import QtQuick

Item {
    id: root

    property bool checked: false
    property bool enabled: true

    // Never flips itself: it asks, and `checked` follows the stored setting. A
    // switch that moved on its own would lie whenever a write is refused.
    signal requested(bool value)

    implicitWidth: 42
    implicitHeight: 24

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: root.checked ? Theme.accent : Theme.track
        border.width: 1
        border.color: root.checked ? Theme.accent : Theme.border
        opacity: root.enabled ? 1 : 0.5

        Behavior on color {
            ColorAnimation { duration: 110 }
        }

        Rectangle {
            x: root.checked ? parent.width - width - 3 : 3
            y: 3
            width: parent.height - 6
            height: parent.height - 6
            radius: height / 2
            color: root.checked ? "#ffffff" : Theme.surfaceRaised
            border.width: root.checked ? 0 : 1
            border.color: Theme.border

            Behavior on x {
                NumberAnimation { duration: 110; easing.type: Easing.OutCubic }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        anchors.margins: -6
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: root.requested(!root.checked)
    }
}

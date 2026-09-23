import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string heading: ""
    property string value: ""
    property color accent: "transparent"

    implicitWidth: column.implicitWidth + 20
    implicitHeight: column.implicitHeight + 14

    visible: opacity > 0.01
    color: Theme.tooltip
    radius: Theme.radiusSmall
    antialiasing: true

    ColumnLayout {
        id: column
        anchors.centerIn: parent
        spacing: 2

        Text {
            text: root.heading
            color: Qt.rgba(1, 1, 1, 0.72)
            font.pixelSize: Theme.fontMicro
        }

        RowLayout {
            spacing: Theme.gapTight

            Rectangle {
                visible: root.accent.a > 0
                implicitWidth: 8
                implicitHeight: 8
                radius: 4
                color: root.accent
            }

            Text {
                text: root.value
                color: Theme.tooltipInk
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }
        }
    }

    Behavior on opacity {
        NumberAnimation { duration: 90 }
    }
}

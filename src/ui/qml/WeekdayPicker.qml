import QtQuick
import QtQuick.Layouts

RowLayout {
    id: root

    property int days: 0
    property bool editable: true
    property int chipWidth: 40

    signal dayToggled(int dayOfWeek)

    spacing: 4
    opacity: root.editable ? 1 : 0.5

    Repeater {
        model: 7

        Rectangle {
            id: chip

            required property int index

            readonly property int dayOfWeek: index + 1
            readonly property bool selected: (root.days & (1 << index)) !== 0

            implicitWidth: root.chipWidth
            implicitHeight: Theme.controlHeight + 2
            radius: Theme.radiusSmall
            color: chip.selected ? Theme.accent : mouse.containsMouse ? Theme.hover : "transparent"
            border.width: 1
            border.color: chip.selected ? Theme.accent : Theme.border

            Behavior on color {
                ColorAnimation { duration: 90 }
            }

            Text {
                anchors.centerIn: parent
                text: settingsController.weekdayLabels[chip.index]
                color: chip.selected ? "#ffffff" : Theme.inkMuted
                font.pixelSize: Theme.fontLabel
                font.weight: chip.selected ? Font.DemiBold : Font.Normal
            }

            MouseArea {
                id: mouse
                anchors.fill: parent
                hoverEnabled: true
                enabled: root.editable
                cursorShape: Qt.PointingHandCursor
                onClicked: root.dayToggled(chip.dayOfWeek)
            }
        }
    }
}

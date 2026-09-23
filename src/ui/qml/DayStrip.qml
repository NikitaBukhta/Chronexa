import QtQuick

Item {
    id: root

    property var dayModel: null
    property date selectedDay: new Date()

    signal daySelected(date value)

    readonly property int dayCount: dayModel ? dayModel.count : 0
    readonly property real scaleSeconds: Theme.niceCeilSeconds(dayModel ? dayModel.peakSeconds : 0)
    readonly property real slotWidth: dayCount > 0 ? width / dayCount : 0
    readonly property int barHeight: 54
    readonly property int labelHeight: 18

    implicitHeight: barHeight + labelHeight + 4
    implicitWidth: 320

    function sameDay(a, b) {
        return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth() && a.getDate() === b.getDate();
    }

    Rectangle {
        y: root.barHeight
        width: parent.width
        height: 1
        color: Theme.axis
    }

    Repeater {
        model: root.dayModel

        Item {
            id: slot

            required property int index
            required property real seconds
            required property date start
            required property string durationText
            required property string description
            required property bool isCurrent

            readonly property bool isSelected: root.sameDay(start, root.selectedDay)
            readonly property real barThickness: Math.max(3, Math.min(18, root.slotWidth - 3))
            readonly property real filledHeight: root.scaleSeconds > 0 && seconds > 0 ? Math.max(3, (seconds / root.scaleSeconds) * (root.barHeight - 6)) : 0

            x: index * root.slotWidth
            width: root.slotWidth
            height: root.implicitHeight

            Rectangle {
                x: (slot.width - slot.barThickness) / 2 - 3
                y: 0
                width: slot.barThickness + 6
                height: root.barHeight
                radius: Theme.radiusSmall
                color: slot.isSelected ? Theme.hover : mouse.containsMouse ? Theme.hover : "transparent"
                opacity: slot.isSelected ? 1 : 0.6
            }

            Rectangle {
                x: (slot.width - slot.barThickness) / 2
                y: root.barHeight - slot.filledHeight
                width: slot.barThickness
                height: slot.filledHeight
                radius: 4
                color: Theme.accent
                opacity: slot.isSelected ? 1 : mouse.containsMouse ? 0.85 : 0.5
                antialiasing: true

                Behavior on opacity {
                    NumberAnimation { duration: 90 }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: Math.min(4, parent.height)
                    color: parent.color
                }
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: root.barHeight
                width: slot.barThickness + 6
                height: 2
                visible: slot.isSelected
                color: Theme.accent
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                y: root.barHeight + 4
                visible: width <= root.slotWidth
                text: slot.start.getDate()
                color: slot.isSelected ? Theme.ink : slot.isCurrent ? Theme.inkMuted : Theme.inkFaint
                font.pixelSize: Theme.fontMicro
                font.weight: slot.isSelected || slot.isCurrent ? Font.DemiBold : Font.Normal
            }

            MouseArea {
                id: mouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.daySelected(slot.start)
            }

            ChartTooltip {
                heading: slot.description
                value: slot.seconds > 0 ? slot.durationText : qsTr("No activity")
                opacity: mouse.containsMouse ? 1 : 0
                x: Math.max(-slot.x, Math.min(root.width - slot.x - width, (slot.width - width) / 2))
                y: -height - 4
            }
        }
    }
}

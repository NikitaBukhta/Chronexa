import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root

    property var progressModel: null

    readonly property int statusWidth: 150
    readonly property int amountWidth: 110

    spacing: 0

    Repeater {
        model: root.progressModel

        delegate: Item {
            id: row

            required property string category
            required property string kind
            required property string kindText
            required property real fraction
            required property string goalState
            required property string amountText
            required property string statusText
            required property int colorSlot

            readonly property bool bad: row.goalState === "exceeded"
            readonly property bool done: row.goalState === "reached"
            readonly property color barColor: row.bad ? Theme.bad : row.done ? Theme.good : Theme.series(row.colorSlot)

            Layout.fillWidth: true
            implicitHeight: Theme.rowHeight + 10

            RowLayout {
                anchors.fill: parent
                spacing: Theme.gap

                Rectangle {
                    Layout.preferredWidth: 9
                    Layout.preferredHeight: 9
                    radius: 4.5
                    color: Theme.series(row.colorSlot)
                }

                // Not fillWidth, though its texts are: a layout inherits
                // fillWidth from its children and would starve the bar.
                ColumnLayout {
                    Layout.fillWidth: false
                    Layout.preferredWidth: 170
                    spacing: 0

                    Text {
                        Layout.fillWidth: true
                        text: row.category
                        color: Theme.ink
                        font.pixelSize: Theme.fontBody
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: row.kindText
                        color: Theme.inkFaint
                        font.pixelSize: Theme.fontMicro
                        elide: Text.ElideRight
                    }
                }

                // The track is the threshold; a limit gone past fills it and
                // turns red rather than growing out of the card.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 8
                    radius: 4
                    color: Theme.track

                    Rectangle {
                        width: Math.max(row.fraction > 0 ? 4 : 0, parent.width * Math.min(1, row.fraction))
                        height: parent.height
                        radius: 4
                        color: row.barColor

                        Behavior on width {
                            NumberAnimation { duration: 240; easing.type: Easing.OutCubic }
                        }
                    }
                }

                Text {
                    Layout.preferredWidth: root.amountWidth
                    horizontalAlignment: Text.AlignRight
                    text: row.amountText
                    color: Theme.ink
                    font.pixelSize: Theme.fontBody
                    elide: Text.ElideRight
                }

                Text {
                    Layout.preferredWidth: root.statusWidth
                    horizontalAlignment: Text.AlignRight
                    text: row.statusText
                    color: row.bad ? Theme.bad : row.done ? Theme.good : Theme.inkMuted
                    font.pixelSize: Theme.fontLabel
                    font.weight: row.bad || row.done ? Font.DemiBold : Font.Normal
                    elide: Text.ElideRight
                }
            }
        }
    }
}

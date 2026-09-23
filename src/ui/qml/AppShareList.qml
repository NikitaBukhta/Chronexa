import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root

    property var totalsModel: null

    property string nameHeader: qsTr("Application")

    property int visibleRows: 0

    readonly property int rowCount: totalsModel ? totalsModel.count : 0

    // Shared column geometry, for the same reason as in SessionTable.
    readonly property int edge: 2
    readonly property int columnGap: Theme.gap
    readonly property int timeWidth: 78
    readonly property int shareWidth: 46
    // The bar only restates the percentage, so it gives way first and the
    // name keeps room to be read; a fixed bar overflowed narrow cards.
    readonly property int flexibleWidth: Math.max(0, width - edge * 2 - timeWidth - shareWidth - columnGap * 3)
    readonly property int barWidth: Math.max(0, Math.min(132, flexibleWidth - 120))
    readonly property int nameWidth: flexibleWidth - barWidth

    readonly property int nameX: edge
    readonly property int timeX: nameX + nameWidth + columnGap
    readonly property int shareX: timeX + timeWidth + columnGap
    readonly property int barX: shareX + shareWidth + columnGap

    implicitHeight: header.height + list.height + 4

    Item {
        id: header

        width: parent.width
        height: 22

        Text {
            x: root.nameX
            width: root.nameWidth
            text: root.nameHeader
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
            elide: Text.ElideRight
        }

        Text {
            x: root.timeX
            width: root.timeWidth
            horizontalAlignment: Text.AlignRight
            text: qsTr("Time")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
        }

        Text {
            x: root.shareX
            width: root.shareWidth
            horizontalAlignment: Text.AlignRight
            text: qsTr("Share")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
        }
    }

    ListView {
        id: list

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.visibleRows > 0 ? Math.min(root.rowCount, root.visibleRows) * Theme.rowHeight : contentHeight
        model: root.totalsModel
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: root.visibleRows > 0 && root.rowCount > root.visibleRows

        ScrollBar.vertical: ScrollBar {
            policy: list.interactive ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        }

        delegate: Rectangle {
            id: row

            required property int index
            required property string appName
            required property string durationText
            required property string sharePercentText
            required property real share
            required property int sessionCount
            required property int colorSlot

            readonly property color slotColor: Theme.series(colorSlot)

            width: list.width
            height: Theme.rowHeight
            color: hover.hovered ? Theme.hover : "transparent"
            radius: Theme.radiusSmall

            HoverHandler {
                id: hover
            }

            Rectangle {
                x: root.nameX
                anchors.verticalCenter: parent.verticalCenter
                width: 9
                height: 9
                radius: 4.5
                color: row.slotColor
            }

            Text {
                x: root.nameX + 17
                width: root.nameWidth - 17
                anchors.verticalCenter: parent.verticalCenter
                text: row.appName
                color: Theme.ink
                font.pixelSize: Theme.fontBody
                elide: Text.ElideRight
            }

            Text {
                x: root.timeX
                width: root.timeWidth
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignRight
                text: row.durationText
                color: Theme.ink
                font.pixelSize: Theme.fontBody
            }

            Text {
                x: root.shareX
                width: root.shareWidth
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignRight
                text: row.sharePercentText
                color: Theme.inkMuted
                font.pixelSize: Theme.fontLabel
            }

            Rectangle {
                x: root.barX
                anchors.verticalCenter: parent.verticalCenter
                visible: root.barWidth >= 24
                width: root.barWidth
                height: 8
                radius: 4
                color: Theme.track

                Rectangle {
                    width: Math.max(row.share > 0 ? 4 : 0, parent.width * row.share)
                    height: parent.height
                    radius: 4
                    color: row.slotColor
                }
            }
        }
    }
}

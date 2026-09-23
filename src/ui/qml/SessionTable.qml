import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root

    property var sessionModel: null
    property string filter: ""
    property bool showDay: false

    // Columns come from one shared set of x/width values: letting every row lay
    // itself out makes them drift with the length of the text inside them.
    readonly property int edge: 2
    readonly property int columnGap: Theme.gap
    readonly property int whenWidth: showDay ? 156 : 112
    readonly property int durationWidth: 76
    // What is left for the two text columns. They split it rather than taking
    // fixed widths: in a half-width card at 125% scaling the fixed sum was
    // wider than the card and pushed Duration past its right edge.
    readonly property int flexibleWidth: Math.max(0, width - edge * 2 - whenWidth - durationWidth - columnGap * 3)
    readonly property int appWidth: Math.min(176, Math.round(flexibleWidth / 2))
    readonly property int windowWidth: Math.max(0, flexibleWidth - appWidth)

    readonly property int whenX: edge
    readonly property int appX: whenX + whenWidth + columnGap
    readonly property int windowX: appX + appWidth + columnGap
    readonly property int durationX: windowX + windowWidth + columnGap

    implicitHeight: 22 + 5

    Item {
        id: header

        width: parent.width
        height: 22

        Text {
            x: root.whenX
            width: root.whenWidth
            text: qsTr("When")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
            elide: Text.ElideRight
        }

        Text {
            x: root.appX
            width: root.appWidth
            text: qsTr("Application")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
            elide: Text.ElideRight
        }

        Text {
            x: root.windowX
            width: root.windowWidth
            text: qsTr("Window")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
            elide: Text.ElideRight
        }

        Text {
            x: root.durationX
            width: root.durationWidth
            horizontalAlignment: Text.AlignRight
            text: qsTr("Duration")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
            elide: Text.ElideRight
        }
    }

    Rectangle {
        anchors.top: header.bottom
        width: parent.width
        height: 1
        color: Theme.grid
    }

    ListView {
        id: list

        anchors.top: header.bottom
        anchors.topMargin: 5
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        model: root.sessionModel

        ScrollBar.vertical: ScrollBar {}

        delegate: Rectangle {
            id: row

            required property int index
            required property string appName
            required property string title
            required property string durationText
            required property string startedText
            required property string endedText
            required property string dayText
            required property int colorSlot

            readonly property bool matches: root.filter === "" || appName.toLowerCase().includes(root.filter.toLowerCase()) || title.toLowerCase().includes(root.filter.toLowerCase())

            width: list.width
            height: matches ? Theme.rowHeight : 0
            visible: matches
            color: hover.hovered ? Theme.hover : "transparent"
            radius: Theme.radiusSmall

            HoverHandler {
                id: hover
            }

            Text {
                x: root.whenX
                width: root.whenWidth
                anchors.verticalCenter: parent.verticalCenter
                text: (root.showDay ? row.dayText + ", " : "") + row.startedText + " – " + row.endedText
                color: Theme.inkMuted
                font.pixelSize: Theme.fontLabel
                elide: Text.ElideRight
            }

            Rectangle {
                x: root.appX
                anchors.verticalCenter: parent.verticalCenter
                width: 9
                height: 9
                radius: 4.5
                color: Theme.series(row.colorSlot)
            }

            Text {
                x: root.appX + 17
                width: root.appWidth - 17
                anchors.verticalCenter: parent.verticalCenter
                text: row.appName
                color: Theme.ink
                font.pixelSize: Theme.fontBody
                elide: Text.ElideRight
            }

            Text {
                x: root.windowX
                width: root.windowWidth
                anchors.verticalCenter: parent.verticalCenter
                text: row.title
                color: Theme.inkMuted
                font.pixelSize: Theme.fontLabel
                elide: Text.ElideRight
            }

            Text {
                x: root.durationX
                width: root.durationWidth
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignRight
                text: row.durationText
                color: Theme.ink
                font.pixelSize: Theme.fontBody
            }
        }
    }
}

import QtQuick

Item {
    id: root

    property var sessionModel: null
    property date dayStart: new Date()
    property int bandHeight: 34
    property int axisHeight: 22

    property int readoutHeight: 36

    readonly property real spanMs: 24 * 60 * 60 * 1000
    readonly property real startMs: dayStart.getTime()

    property int hoveredIndex: -1
    property real hoveredCenter: 0
    property string hoveredHeading: ""
    property string hoveredValue: ""
    property color hoveredAccent: "transparent"

    implicitHeight: bandHeight + axisHeight + readoutHeight
    implicitWidth: 320

    Rectangle {
        id: band

        width: parent.width
        height: root.bandHeight
        radius: Theme.radiusSmall
        color: Theme.surfaceSunken
        clip: true

        Repeater {
            model: 8

            Rectangle {
                required property int index

                visible: index > 0
                x: Math.round(band.width * index / 8)
                width: 1
                height: band.height
                color: Theme.grid
            }
        }

        Repeater {
            model: root.sessionModel

            Rectangle {
                id: segment

                required property int index
                required property date startedOn
                required property date endedOn
                required property string appName
                required property string durationText
                required property string startedText
                required property string endedText
                required property int colorSlot

                readonly property real fromFraction: (startedOn.getTime() - root.startMs) / root.spanMs
                readonly property real toFraction: (endedOn.getTime() - root.startMs) / root.spanMs

                x: Math.round(band.width * Math.max(0, fromFraction))
                // The pixel taken off the width is the surface gap that keeps
                // back-to-back sessions from merging into one block.
                width: Math.max(2, Math.round(band.width * (Math.min(1, toFraction) - Math.max(0, fromFraction))) - 1)
                height: band.height
                color: Theme.series(colorSlot)
                opacity: root.hoveredIndex >= 0 && root.hoveredIndex !== index ? 0.4 : 1

                Behavior on opacity {
                    NumberAnimation { duration: 90 }
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -6
                    hoverEnabled: true

                    onEntered: {
                        root.hoveredIndex = segment.index;
                        root.hoveredCenter = segment.x + segment.width / 2;
                        root.hoveredHeading = segment.startedText + " – " + segment.endedText;
                        root.hoveredValue = segment.appName + "  ·  " + segment.durationText;
                        root.hoveredAccent = segment.color;
                    }
                    onExited: {
                        if (root.hoveredIndex === segment.index)
                            root.hoveredIndex = -1;
                    }
                }
            }
        }
    }

    Repeater {
        model: 9

        Text {
            required property int index

            readonly property real fraction: index / 8

            x: Math.round((band.width - width) * fraction)
            y: root.bandHeight + 6
            text: (index * 3).toString().padStart(2, "0")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
        }
    }

    Text {
        y: root.bandHeight + root.axisHeight + 12
        width: root.width
        horizontalAlignment: Text.AlignHCenter
        visible: root.hoveredIndex < 0 && root.sessionModel && root.sessionModel.count > 0
        text: qsTr("Point at a block to see which window it was")
        color: Theme.inkFaint
        font.pixelSize: Theme.fontMicro
    }

    ChartTooltip {
        heading: root.hoveredHeading
        value: root.hoveredValue
        accent: root.hoveredAccent
        opacity: root.hoveredIndex >= 0 ? 1 : 0
        x: Math.max(0, Math.min(root.width - width, root.hoveredCenter - width / 2))
        y: root.bandHeight + root.axisHeight + 2
    }
}

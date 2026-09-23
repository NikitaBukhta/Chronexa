import QtQuick

Item {
    id: root

    property var barModel: null
    property int plotHeight: 190
    property int axisHeight: 26
    property int yAxisWidth: 52
    property color barColor: Theme.accent
    property int maxBarThickness: 24

    readonly property int barCount: barModel ? barModel.count : 0
    readonly property real scaleSeconds: Theme.niceCeilSeconds(barModel ? barModel.peakSeconds : 0)
    readonly property real slotWidth: barCount > 0 ? plot.width / barCount : 0

    readonly property int labelStride: slotWidth >= 26 ? 1 : slotWidth >= 15 ? 2 : slotWidth >= 10 ? 3 : 6

    property int hoveredIndex: -1
    property real hoveredCenter: 0
    property real hoveredTop: 0
    property string hoveredHeading: ""
    property string hoveredValue: ""

    implicitWidth: 320
    implicitHeight: plotHeight + axisHeight

    Item {
        id: yAxis

        width: root.yAxisWidth
        height: root.plotHeight

        Repeater {
            model: [1, 0.5, 0]

            Text {
                required property real modelData

                y: root.plotHeight - modelData * root.plotHeight - height / 2
                width: yAxis.width - 10
                horizontalAlignment: Text.AlignRight
                // Reads peakSeconds rather than scaleSeconds: the model re-announces
                // it on every refresh, a language switch included, while
                // scaleSeconds only notifies when the value itself moves.
                text: modelData === 0 ? "0" : activityController.formatDuration(Math.round(Theme.niceCeilSeconds(root.barModel ? root.barModel.peakSeconds : 0) * modelData), true)
                color: Theme.inkFaint
                font.pixelSize: Theme.fontMicro
            }
        }
    }

    Item {
        id: plot

        x: root.yAxisWidth
        width: root.width - root.yAxisWidth
        height: root.plotHeight

        Repeater {
            model: [1, 0.5]

            Rectangle {
                required property real modelData

                y: Math.round(plot.height - modelData * plot.height)
                width: plot.width
                height: 1
                color: Theme.grid
            }
        }

        Rectangle {
            y: plot.height - 1
            width: plot.width
            height: 1
            color: Theme.axis
        }

        Repeater {
            model: root.barModel

            Item {
                id: slot

                required property int index
                required property real seconds
                required property string label
                required property string description
                required property string durationText
                required property bool isCurrent

                readonly property real barThickness: Math.max(3, Math.min(root.maxBarThickness, root.slotWidth - 2))
                readonly property real barHeight: root.scaleSeconds > 0 && seconds > 0 ? Math.max(3, (seconds / root.scaleSeconds) * (plot.height - 8)) : 0
                readonly property bool isPeak: seconds > 0 && root.barModel && seconds === root.barModel.peakSeconds
                readonly property bool dimmed: root.hoveredIndex >= 0 && root.hoveredIndex !== index

                x: index * root.slotWidth
                width: root.slotWidth
                height: plot.height

                Rectangle {
                    id: bar

                    x: (slot.width - slot.barThickness) / 2
                    y: plot.height - 1 - slot.barHeight
                    width: slot.barThickness
                    height: slot.barHeight
                    radius: 4
                    color: root.barColor
                    opacity: slot.dimmed ? 0.4 : 1
                    antialiasing: true

                    Behavior on opacity {
                        NumberAnimation { duration: 90 }
                    }

                    // Squares off the end sitting on the baseline, so only the
                    // data end of the bar keeps its radius.
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: Math.min(4, parent.height)
                        color: parent.color
                    }
                }

                Text {
                    id: peakLabel

                    anchors.horizontalCenter: bar.horizontalCenter
                    anchors.bottom: bar.top
                    anchors.bottomMargin: 4
                    visible: slot.isPeak && slot.width >= 34 && width <= slot.width
                    // Spelled-out units ("4 мин 12 с") outgrow a narrow slot;
                    // drop the seconds before dropping the label.
                    text: fullPeakText.advanceWidth <= slot.width ? slot.durationText : activityController.formatDuration(slot.seconds, true)
                    color: Theme.inkMuted
                    font.pixelSize: Theme.fontMicro
                }

                TextMetrics {
                    id: fullPeakText

                    font: peakLabel.font
                    text: slot.durationText
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: plot.height + 7
                    visible: slot.index % root.labelStride === 0 && width <= root.slotWidth + 4
                    text: slot.label
                    color: slot.isCurrent ? Theme.ink : Theme.inkFaint
                    font.pixelSize: Theme.fontMicro
                    font.weight: slot.isCurrent ? Font.DemiBold : Font.Normal
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true

                    onEntered: {
                        root.hoveredIndex = slot.index;
                        root.hoveredCenter = root.yAxisWidth + slot.x + slot.width / 2;
                        root.hoveredTop = bar.y;
                        root.hoveredHeading = slot.description;
                        root.hoveredValue = slot.seconds > 0 ? slot.durationText : qsTr("No activity");
                    }
                    onExited: {
                        if (root.hoveredIndex === slot.index)
                            root.hoveredIndex = -1;
                    }
                }
            }
        }
    }

    ChartTooltip {
        heading: root.hoveredHeading
        value: root.hoveredValue
        opacity: root.hoveredIndex >= 0 ? 1 : 0
        x: Math.max(0, Math.min(root.width - width, root.hoveredCenter - width / 2))
        y: Math.max(0, root.hoveredTop - height - 8)
    }
}

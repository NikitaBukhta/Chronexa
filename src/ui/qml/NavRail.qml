import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property int currentIndex: 0

    readonly property var pages: [
        { glyph: "▤", label: qsTr("Day"), hint: qsTr("One day, hour by hour") },
        { glyph: "∑", label: qsTr("Period"), hint: qsTr("Totals over any range") },
        { glyph: "≡", label: qsTr("Log"), hint: qsTr("Every recorded session") },
        { glyph: "⚙", label: qsTr("Settings"), hint: qsTr("Tracking, start-up, language") }
    ]

    implicitWidth: 214
    color: Theme.surface

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.border
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.gap
        spacing: Theme.gap

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 6
            Layout.leftMargin: 6
            spacing: 1

            Text {
                text: "Chronexa"
                color: Theme.ink
                font.pixelSize: 17
                font.weight: Font.DemiBold
                font.letterSpacing: 0.2
            }

            Text {
                text: qsTr("Where your time went")
                color: Theme.inkFaint
                font.pixelSize: Theme.fontMicro
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.gapTight
            spacing: 2

            Repeater {
                model: root.pages

                Rectangle {
                    id: item

                    required property int index
                    required property var modelData

                    readonly property bool active: root.currentIndex === index

                    Layout.fillWidth: true
                    implicitHeight: 42
                    radius: Theme.radiusSmall
                    color: item.active ? Theme.selected : hover.hovered ? Theme.hover : "transparent"

                    HoverHandler {
                        id: hover
                        cursorShape: Qt.PointingHandCursor
                    }

                    TapHandler {
                        onTapped: root.currentIndex = item.index
                    }

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        x: 0
                        width: 3
                        height: parent.height - 14
                        radius: 2
                        visible: item.active
                        color: Theme.accent
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 10
                        spacing: 10

                        Text {
                            text: item.modelData.glyph
                            color: item.active ? Theme.ink : Theme.inkFaint
                            font.pixelSize: 15
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Text {
                                Layout.fillWidth: true
                                text: item.modelData.label
                                color: item.active ? Theme.ink : Theme.inkMuted
                                font.pixelSize: Theme.fontBody
                                font.weight: item.active ? Font.DemiBold : Font.Normal
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.fillWidth: true
                                text: item.modelData.hint
                                color: Theme.inkFaint
                                font.pixelSize: Theme.fontMicro
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.border
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            Layout.bottomMargin: 4
            spacing: 3

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: 8
                    implicitHeight: 8
                    radius: 4
                    color: activityController.tracking ? Theme.good : activityController.trackingEnabled ? Theme.warning : Theme.inkFaint
                }

                Text {
                    Layout.fillWidth: true
                    text: activityController.tracking ? qsTr("Recording") : activityController.trackingEnabled ? qsTr("On hold") : qsTr("Stopped")
                    color: Theme.inkMuted
                    font.pixelSize: Theme.fontLabel
                    elide: Text.ElideRight
                }
            }

            Text {
                Layout.fillWidth: true
                visible: text !== ""
                text: activityController.heldBySchedule ? activityController.scheduleHoldText : appSettings.scheduleEnabled ? settingsController.scheduleSummary : ""
                color: Theme.inkFaint
                font.pixelSize: Theme.fontMicro
                wrapMode: Text.WordWrap
            }
        }
    }
}

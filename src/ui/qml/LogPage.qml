import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root

    required property var query

    readonly property var presets: [
        { key: "today", label: qsTr("Today") },
        { key: "last7days", label: qsTr("7 days") },
        { key: "last30days", label: qsTr("30 days") },
        { key: "alltime", label: qsTr("All time") }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.gapLoose
        spacing: Theme.gap

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            SegmentedControl {
                options: root.presets
                current: root.query.preset
                onPicked: function (key) {
                    root.query.applyPreset(key);
                }
            }

            Rectangle {
                Layout.preferredWidth: 260
                Layout.preferredHeight: Theme.controlHeight + 6
                radius: Theme.radiusSmall
                color: Theme.surfaceSunken
                border.width: 1
                border.color: search.activeFocus ? Theme.accent : Theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 6
                    spacing: 6

                    Text {
                        text: "⌕"
                        color: Theme.inkFaint
                        font.pixelSize: Theme.fontTitle
                    }

                    TextField {
                        id: search

                        Layout.fillWidth: true
                        placeholderText: qsTr("Filter by application or window")
                        placeholderTextColor: Theme.inkFaint
                        color: Theme.ink
                        font.pixelSize: Theme.fontBody
                        verticalAlignment: Text.AlignVCenter
                        selectByMouse: true
                        background: null
                    }

                    ToolButtonBase {
                        glyph: "✕"
                        compact: true
                        visible: search.text !== ""
                        onClicked: search.clear()
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                text: root.query.empty ? qsTr("No sessions") : qsTr("%1 sessions · %2").arg(root.query.sessionCount).arg(activityController.formatDuration(root.query.totalSeconds, true))
                color: Theme.inkMuted
                font.pixelSize: Theme.fontLabel
            }
        }

        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: qsTr("Sessions")
            subtitle: root.query.rangeLabel

            SessionTable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                sessionModel: root.query.sessions
                filter: search.text
                showDay: true
            }
        }
    }
}

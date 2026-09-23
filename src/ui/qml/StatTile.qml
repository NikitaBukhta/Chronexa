import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string label: ""
    property string value: ""
    property string detail: ""
    property color accent: "transparent"
    property bool emphasised: false

    implicitWidth: 150
    implicitHeight: column.implicitHeight + Theme.pad * 2

    color: Theme.surface
    radius: Theme.radius
    border.width: 1
    border.color: root.emphasised ? Theme.accent : Theme.border

    ColumnLayout {
        id: column

        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: 6

        Text {
            Layout.fillWidth: true
            text: root.label
            color: Theme.inkFaint
            font.pixelSize: Theme.fontLabel
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: root.value
            color: Theme.ink
            font.pixelSize: Theme.fontDisplay
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.detail !== ""
            spacing: Theme.gapTight

            Rectangle {
                visible: root.accent.a > 0
                implicitWidth: 8
                implicitHeight: 8
                radius: 4
                color: root.accent
            }

            Text {
                Layout.fillWidth: true
                text: root.detail
                color: Theme.inkMuted
                font.pixelSize: Theme.fontLabel
                elide: Text.ElideRight
            }
        }
    }
}

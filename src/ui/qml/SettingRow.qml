import QtQuick
import QtQuick.Layouts

RowLayout {
    id: root

    property string label: ""
    property string explanation: ""
    property bool enabledRow: true

    default property alias control: controlHolder.data

    spacing: Theme.gapLoose
    opacity: root.enabledRow ? 1 : 0.5

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Text {
            Layout.fillWidth: true
            text: root.label
            color: Theme.ink
            font.pixelSize: Theme.fontBody
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            visible: root.explanation !== ""
            text: root.explanation
            color: Theme.inkFaint
            font.pixelSize: Theme.fontLabel
            wrapMode: Text.WordWrap
        }
    }

    RowLayout {
        id: controlHolder
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
        spacing: Theme.gapTight
    }
}

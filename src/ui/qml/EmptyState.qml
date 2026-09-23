import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root

    property string headline: qsTr("No activity recorded")
    property string explanation: ""

    spacing: 6

    Text {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        horizontalAlignment: Text.AlignHCenter
        text: root.headline
        color: Theme.inkMuted
        font.pixelSize: Theme.fontBody
        font.weight: Font.DemiBold
    }

    Text {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        horizontalAlignment: Text.AlignHCenter
        visible: root.explanation !== ""
        text: root.explanation
        color: Theme.inkFaint
        font.pixelSize: Theme.fontLabel
        wrapMode: Text.WordWrap
    }
}

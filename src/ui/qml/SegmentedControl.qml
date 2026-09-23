import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property var options: []
    property string current: ""

    signal picked(string key)

    implicitWidth: row.implicitWidth + 6
    implicitHeight: Theme.controlHeight + 6

    color: Theme.surfaceSunken
    radius: Theme.radiusSmall
    border.width: 1
    border.color: Theme.border

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: 3
        spacing: 2

        Repeater {
            model: root.options

            ToolButtonBase {
                required property var modelData

                Layout.fillHeight: true
                text: modelData.label
                active: modelData.key === root.current
                compact: true
                onClicked: root.picked(modelData.key)
            }
        }
    }
}

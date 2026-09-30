import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ToolButtonBase {
    id: root

    property string current: ""
    property var names: []
    property bool invalid: false

    signal picked(string name)

    text: root.current !== "" ? root.current : qsTr("Pick a category")
    glyph: "▾"
    glyphTrailing: true
    onClicked: menu.open()

    // Looks like the TextBox next to it: it is a field, not an action. A
    // category no rule produces any more is marked the same way.
    background: Rectangle {
        radius: Theme.radiusSmall
        color: root.down ? Theme.pressed : root.hovered ? Theme.hover : Theme.surfaceSunken
        border.width: 1
        border.color: root.invalid ? Theme.warning : menu.visible ? Theme.accent : Theme.border
    }

    Popup {
        id: menu

        y: root.height + 4
        padding: Theme.gapTight
        focus: true
        implicitWidth: Math.max(root.width, column.implicitWidth + padding * 2)

        background: Rectangle {
            color: Theme.surfaceRaised
            radius: Theme.radius
            border.width: 1
            border.color: Theme.border
        }

        ColumnLayout {
            id: column

            width: parent.width
            spacing: 2

            Repeater {
                model: root.names

                ToolButtonBase {
                    required property string modelData

                    Layout.fillWidth: true
                    text: modelData
                    compact: true
                    active: modelData === root.current
                    onClicked: {
                        menu.close();
                        root.picked(modelData);
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.names.length === 0
                text: qsTr("No categories yet — add a rule above.")
                color: Theme.inkFaint
                font.pixelSize: Theme.fontLabel
                wrapMode: Text.WordWrap
            }
        }
    }
}

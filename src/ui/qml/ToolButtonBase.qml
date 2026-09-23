import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root

    property bool active: false
    property string glyph: ""

    property bool glyphTrailing: false

    property bool compact: false

    implicitHeight: Theme.controlHeight
    padding: root.compact ? 8 : 12
    hoverEnabled: true
    font.pixelSize: Theme.fontBody

    background: Rectangle {
        radius: Theme.radiusSmall
        color: root.active ? Theme.selected : root.down ? Theme.pressed : root.hovered ? Theme.hover : "transparent"
        border.width: 1
        border.color: root.active ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.55) : root.hovered ? Theme.border : "transparent"
    }

    contentItem: Row {
        spacing: root.text !== "" && root.glyph !== "" ? 6 : 0

        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.glyph !== "" && !root.glyphTrailing
            text: root.glyph
            color: root.active ? Theme.ink : Theme.inkMuted
            font.pixelSize: Theme.fontBody
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.text !== ""
            text: root.text
            color: root.active ? Theme.ink : Theme.inkMuted
            font: root.font
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.glyph !== "" && root.glyphTrailing
            text: root.glyph
            color: root.active ? Theme.ink : Theme.inkFaint
            font.pixelSize: Theme.fontMicro
        }
    }
}

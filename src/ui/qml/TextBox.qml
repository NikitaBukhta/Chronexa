import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: root

    property string value: ""
    property string placeholder: ""
    property bool invalid: false

    signal edited(string value)

    implicitWidth: 140
    implicitHeight: Theme.controlHeight

    radius: Theme.radiusSmall
    color: Theme.surfaceSunken
    border.width: 1
    border.color: root.invalid ? Theme.warning : field.activeFocus ? Theme.accent : Theme.border

    // Re-synced only while not being typed into, so an update arriving from the
    // model never yanks the text out from under the cursor.
    function reset() {
        field.text = root.value;
    }

    onValueChanged: if (!field.activeFocus)
        reset()

    Component.onCompleted: reset()

    TextField {
        id: field

        anchors.fill: parent
        anchors.margins: 1
        leftPadding: Theme.gapTight + 2
        rightPadding: Theme.gapTight + 2
        color: Theme.ink
        placeholderText: root.placeholder
        placeholderTextColor: Theme.inkFaint
        font.pixelSize: Theme.fontBody
        verticalAlignment: Text.AlignVCenter
        selectByMouse: true
        background: null

        onEditingFinished: {
            if (text !== root.value)
                root.edited(text);
            root.reset();
        }
    }
}

import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: root

    property int minutes: 0
    property bool editable: true

    signal edited(int minutes)

    implicitWidth: 74
    implicitHeight: Theme.controlHeight + 6

    radius: Theme.radiusSmall
    color: Theme.surfaceSunken
    border.width: 1
    border.color: field.activeFocus ? Theme.accent : Theme.border
    opacity: root.editable ? 1 : 0.5

    function reset() {
        field.text = settingsController.formatMinutes(root.minutes);
    }

    onMinutesChanged: if (!field.activeFocus)
        reset()

    Component.onCompleted: reset()

    TextField {
        id: field

        anchors.fill: parent
        anchors.margins: 1
        enabled: root.editable
        inputMask: "99:99"
        color: Theme.ink
        font.pixelSize: Theme.fontBody
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        selectByMouse: true
        background: null

        onEditingFinished: {
            const parsed = settingsController.parseMinutes(text);
            if (parsed < 0) {
                root.reset();
                return;
            }
            root.edited(parsed);
            root.reset();
        }
    }
}

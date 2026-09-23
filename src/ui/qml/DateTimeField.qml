import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

RowLayout {
    id: root

    property date value: new Date()
    // Days past today that stay selectable; the popup resolves "today" itself
    // every time it opens.
    property int maximumDayOffset: 0
    property bool showTime: true

    signal edited(date value)

    spacing: 4

    function commit(year, month, day, hours, minutes) {
        const next = new Date(year, month, day, hours, minutes, 0, 0);
        root.edited(next);
    }

    ToolButtonBase {
        text: root.value.toLocaleDateString(Qt.locale(settingsController.resolvedLanguage), Locale.ShortFormat)
        glyph: "▾"
        glyphTrailing: true
        onClicked: calendar.open()

        CalendarPopup {
            id: calendar

            y: parent.height + 4
            selected: root.value
            maximumDayOffset: root.maximumDayOffset

            onPicked: function (picked) {
                root.commit(picked.getFullYear(), picked.getMonth(), picked.getDate(), root.value.getHours(), root.value.getMinutes());
            }
        }
    }

    Rectangle {
        Layout.preferredWidth: 58
        Layout.preferredHeight: Theme.controlHeight
        visible: root.showTime
        radius: Theme.radiusSmall
        color: Theme.surfaceSunken
        border.width: 1
        border.color: time.activeFocus ? Theme.accent : Theme.border

        TextField {
            id: time

            anchors.fill: parent
            anchors.margins: 1
            inputMask: "99:99"
            text: Qt.formatTime(root.value, "HH:mm")
            color: Theme.ink
            font.pixelSize: Theme.fontBody
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            selectByMouse: true
            background: null

            onEditingFinished: {
                const parts = text.split(":");
                const hours = Math.max(0, Math.min(23, parseInt(parts[0], 10) || 0));
                const minutes = Math.max(0, Math.min(59, parseInt(parts[1], 10) || 0));
                root.commit(root.value.getFullYear(), root.value.getMonth(), root.value.getDate(), hours, minutes);
            }
        }
    }
}

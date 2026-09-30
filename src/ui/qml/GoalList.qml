import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    property var goalsModel: null

    readonly property int rowCount: root.goalsModel ? root.goalsModel.count : 0

    readonly property int dotWidth: 9
    readonly property int dayChipWidth: 30

    readonly property var kindOptions: [
        { key: "limit", label: qsTr("At most") },
        { key: "target", label: qsTr("At least") }
    ]

    spacing: Theme.gapTight

    Repeater {
        model: root.goalsModel

        delegate: RowLayout {
            id: row

            required property int index
            required property string category
            required property string kind
            required property int minutes
            required property int days
            required property bool valid
            required property bool knownCategory
            required property int colorSlot

            Layout.fillWidth: true
            spacing: Theme.gapTight

            Rectangle {
                Layout.preferredWidth: root.dotWidth
                Layout.preferredHeight: root.dotWidth
                radius: root.dotWidth / 2
                color: row.valid && row.knownCategory ? Theme.series(row.colorSlot) : "transparent"
                border.width: row.valid && row.knownCategory ? 0 : 1
                border.color: Theme.warning
            }

            CategoryPicker {
                Layout.fillWidth: true
                Layout.minimumWidth: 110
                current: row.category
                names: root.goalsModel ? root.goalsModel.categoryNames : []
                invalid: !row.knownCategory
                onPicked: function (name) {
                    root.goalsModel.setCategory(row.index, name);
                }
            }

            SegmentedControl {
                options: root.kindOptions
                current: row.kind
                onPicked: function (key) {
                    root.goalsModel.setKind(row.index, key);
                }
            }

            MinutesField {
                minutes: row.minutes
                onEdited: function (value) {
                    root.goalsModel.setMinutes(row.index, value);
                }
            }

            Text {
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("a day")
                color: Theme.inkFaint
                font.pixelSize: Theme.fontLabel
            }

            WeekdayPicker {
                chipWidth: root.dayChipWidth
                days: row.days
                onDayToggled: function (dayOfWeek) {
                    root.goalsModel.toggleDay(row.index, dayOfWeek);
                }
            }

            ToolButtonBase {
                Layout.preferredWidth: Theme.controlHeight
                glyph: "✕"
                compact: true
                onClicked: root.goalsModel.removeGoal(row.index)
            }
        }
    }

    Text {
        Layout.fillWidth: true
        visible: root.rowCount === 0
        text: qsTr("No goals — add one, e.g. at most an hour of distractions a day.")
        color: Theme.inkFaint
        font.pixelSize: Theme.fontLabel
        wrapMode: Text.WordWrap
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: Theme.gapTight
        spacing: Theme.gapTight

        ToolButtonBase {
            text: qsTr("Add goal")
            glyph: "+"
            onClicked: root.goalsModel.addGoal()
        }

        Item {
            Layout.fillWidth: true
        }
    }
}

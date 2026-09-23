import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    property var rulesModel: null

    readonly property int rowCount: root.rulesModel ? root.rulesModel.count : 0

    readonly property int categoryWidth: 140
    readonly property int titleWidth: 180
    readonly property int dotWidth: 9

    spacing: Theme.gapTight

    RowLayout {
        Layout.fillWidth: true
        visible: root.rowCount > 0
        spacing: Theme.gapTight

        Item {
            Layout.preferredWidth: root.dotWidth
        }

        Text {
            Layout.preferredWidth: root.categoryWidth
            text: qsTr("Category")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Applications, comma-separated")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
            elide: Text.ElideRight
        }

        Text {
            Layout.preferredWidth: root.titleWidth
            text: qsTr("Window title (regex)")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontMicro
            elide: Text.ElideRight
        }

        // Room for the three row buttons.
        Item {
            Layout.preferredWidth: 3 * Theme.controlHeight + 2 * Theme.gapTight
        }
    }

    Repeater {
        model: root.rulesModel

        delegate: RowLayout {
            id: row

            required property int index
            required property string category
            required property string apps
            required property string titlePattern
            required property bool titleValid
            required property bool valid
            required property int colorSlot

            Layout.fillWidth: true
            spacing: Theme.gapTight

            Rectangle {
                Layout.preferredWidth: root.dotWidth
                Layout.preferredHeight: root.dotWidth
                radius: root.dotWidth / 2
                color: row.valid ? Theme.series(row.colorSlot) : "transparent"
                border.width: row.valid ? 0 : 1
                border.color: Theme.warning
            }

            TextBox {
                Layout.preferredWidth: root.categoryWidth
                value: row.category
                placeholder: qsTr("Name")
                invalid: row.category.trim() === ""
                onEdited: function (value) {
                    root.rulesModel.setCategory(row.index, value);
                }
            }

            TextBox {
                Layout.fillWidth: true
                value: row.apps
                placeholder: qsTr("Any application")
                onEdited: function (value) {
                    root.rulesModel.setApps(row.index, value);
                }
            }

            TextBox {
                Layout.preferredWidth: root.titleWidth
                value: row.titlePattern
                placeholder: qsTr("Any title")
                invalid: !row.titleValid
                onEdited: function (value) {
                    root.rulesModel.setTitlePattern(row.index, value);
                }
            }

            ToolButtonBase {
                Layout.preferredWidth: Theme.controlHeight
                glyph: "↑"
                compact: true
                enabled: row.index > 0
                opacity: enabled ? 1 : 0.35
                onClicked: root.rulesModel.moveRule(row.index, -1)
            }

            ToolButtonBase {
                Layout.preferredWidth: Theme.controlHeight
                glyph: "↓"
                compact: true
                enabled: row.index < root.rowCount - 1
                opacity: enabled ? 1 : 0.35
                onClicked: root.rulesModel.moveRule(row.index, 1)
            }

            ToolButtonBase {
                Layout.preferredWidth: Theme.controlHeight
                glyph: "✕"
                compact: true
                onClicked: root.rulesModel.removeRule(row.index)
            }
        }
    }

    Text {
        Layout.fillWidth: true
        visible: root.rowCount === 0
        text: qsTr("No rules — all time counts as uncategorized.")
        color: Theme.inkFaint
        font.pixelSize: Theme.fontLabel
        wrapMode: Text.WordWrap
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: Theme.gapTight
        spacing: Theme.gapTight

        ToolButtonBase {
            text: qsTr("Add rule")
            glyph: "+"
            onClicked: root.rulesModel.addRule()
        }

        Item {
            Layout.fillWidth: true
        }

        ToolButtonBase {
            text: qsTr("Restore defaults")
            compact: true
            onClicked: root.rulesModel.restoreDefaults()
        }
    }
}

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    property var rulesModel: null

    readonly property int rowCount: root.rulesModel ? root.rulesModel.count : 0

    // Sized from the labels, which are much longer in some languages.
    readonly property int actionWidth: actionProbe.implicitWidth
    readonly property int titleWidth: 180

    readonly property var actionOptions: [
        { key: "exclude", label: qsTr("Don't record") },
        { key: "hideTitle", label: qsTr("Hide title") }
    ]

    spacing: Theme.gapTight

    // Never shown; measures the action picker for the header column.
    SegmentedControl {
        id: actionProbe

        visible: false
        options: root.actionOptions
    }

    RowLayout {
        Layout.fillWidth: true
        visible: root.rowCount > 0
        spacing: Theme.gapTight

        Text {
            Layout.preferredWidth: root.actionWidth
            text: qsTr("Action")
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

        // Room for the remove button.
        Item {
            Layout.preferredWidth: Theme.controlHeight
        }
    }

    Repeater {
        model: root.rulesModel

        delegate: RowLayout {
            id: row

            required property int index
            required property string action
            required property string apps
            required property string titlePattern
            required property bool titleValid
            required property bool valid

            Layout.fillWidth: true
            spacing: Theme.gapTight

            SegmentedControl {
                Layout.preferredWidth: root.actionWidth
                options: root.actionOptions
                current: row.action
                onPicked: function (key) {
                    root.rulesModel.setAction(row.index, key);
                }
            }

            TextBox {
                Layout.fillWidth: true
                value: row.apps
                placeholder: qsTr("Any application")
                // Neither condition set: the rule would apply to nothing.
                invalid: row.apps.trim() === "" && row.titlePattern.trim() === ""
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
                glyph: "✕"
                compact: true
                onClicked: root.rulesModel.removeRule(row.index)
            }
        }
    }

    Text {
        Layout.fillWidth: true
        visible: root.rowCount === 0
        text: qsTr("No rules — every window is recorded with its title.")
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

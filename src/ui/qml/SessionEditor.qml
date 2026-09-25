import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Corrects one session of the log: renames it, files it under a category, or
// removes a stretch of its time. Everything goes through activityController,
// which answers with an empty string or the reason it could not be done.
Dialog {
    id: root

    objectName: "sessionEditor"

    // The row as SessionTable hands it over.
    property var session: ({})
    // Categories the rules define; the picker is hidden without any.
    property var categoryNames: []

    property string categoryMode: "auto"
    property string category: ""
    property date cutFrom: new Date()
    property date cutTo: new Date()
    property string error: ""
    property bool confirmDelete: false

    function openFor(row) {
        session = row;
        appField.value = row.appName;
        titleField.value = row.title;
        appField.reset();
        titleField.reset();
        categoryMode = row.categoryMode;
        category = row.categoryMode === "set" ? row.category : "";
        cutFrom = row.startedOn;
        cutTo = row.endedOn;
        error = "";
        confirmDelete = false;
        open();
    }

    function finish(failure) {
        if (failure === "")
            close();
        else
            error = failure;
    }

    function save() {
        finish(activityController.editSession(session.appName, session.title, session.startedOn, session.endedOn, appField.text, titleField.text, categoryMode, category));
    }

    function cut(from, to) {
        finish(activityController.cutSession(session.appName, session.title, session.startedOn, session.endedOn, from, to));
    }

    width: 460
    modal: true
    padding: Theme.pad
    title: qsTr("Edit session")

    background: Rectangle {
        color: Theme.surfaceRaised
        radius: Theme.radius
        border.width: 1
        border.color: Theme.border
    }

    header: ColumnLayout {
        spacing: 2

        Text {
            Layout.topMargin: Theme.pad
            Layout.leftMargin: Theme.pad
            text: root.title
            color: Theme.ink
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }

        Text {
            Layout.leftMargin: Theme.pad
            text: root.session.dayText !== undefined ? root.session.dayText + ", " + root.session.startedText + " – " + root.session.endedText + " · " + root.session.durationText : ""
            color: Theme.inkFaint
            font.pixelSize: Theme.fontLabel
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.gap

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.gap
            rowSpacing: Theme.gapTight

            Text {
                text: qsTr("Application")
                color: Theme.inkMuted
                font.pixelSize: Theme.fontLabel
            }

            TextBox {
                id: appField

                objectName: "sessionAppField"
                Layout.fillWidth: true
                invalid: text.trim() === ""
                // Kept, or TextBox would reset the field to the old name.
                onEdited: function (value) {
                    appField.value = value;
                }
            }

            Text {
                text: qsTr("Window")
                color: Theme.inkMuted
                font.pixelSize: Theme.fontLabel
            }

            TextBox {
                id: titleField

                objectName: "sessionTitleField"
                Layout.fillWidth: true
                onEdited: function (value) {
                    titleField.value = value;
                }
            }

            Text {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 6
                visible: root.categoryNames.length > 0
                text: qsTr("Category")
                color: Theme.inkMuted
                font.pixelSize: Theme.fontLabel
            }

            Flow {
                Layout.fillWidth: true
                visible: root.categoryNames.length > 0
                spacing: 4

                ToolButtonBase {
                    text: root.categoryMode === "auto" && root.session.category ? qsTr("By rules: %1").arg(root.session.category) : qsTr("By rules")
                    compact: true
                    active: root.categoryMode === "auto"
                    onClicked: root.categoryMode = "auto"
                }

                ToolButtonBase {
                    text: qsTr("None")
                    compact: true
                    active: root.categoryMode === "none"
                    onClicked: root.categoryMode = "none"
                }

                Repeater {
                    model: root.categoryNames

                    ToolButtonBase {
                        required property string modelData

                        text: modelData
                        compact: true
                        active: root.categoryMode === "set" && root.category === modelData
                        onClicked: {
                            root.categoryMode = "set";
                            root.category = modelData;
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gapTight

            Item {
                Layout.fillWidth: true
            }

            ToolButtonBase {
                text: qsTr("Cancel")
                onClicked: root.close()
            }

            ToolButtonBase {
                objectName: "sessionSaveButton"
                text: qsTr("Save")
                active: true
                onClicked: root.save()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.grid
        }

        Text {
            text: qsTr("Remove time")
            color: Theme.ink
            font.pixelSize: Theme.fontBody
            font.weight: Font.DemiBold
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("For when the tracker kept counting after you left: the chosen stretch is taken out of this session and of every total.")
            color: Theme.inkFaint
            font.pixelSize: Theme.fontLabel
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gapTight

            Text {
                text: qsTr("From")
                color: Theme.inkMuted
                font.pixelSize: Theme.fontLabel
            }

            DateTimeField {
                objectName: "cutFromField"
                value: root.cutFrom
                onEdited: function (value) {
                    root.cutFrom = value;
                }
            }

            Text {
                Layout.leftMargin: Theme.gapTight
                text: qsTr("To")
                color: Theme.inkMuted
                font.pixelSize: Theme.fontLabel
            }

            DateTimeField {
                objectName: "cutToField"
                value: root.cutTo
                onEdited: function (value) {
                    root.cutTo = value;
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gapTight

            ToolButtonBase {
                objectName: "sessionDeleteButton"
                text: root.confirmDelete ? qsTr("Click again to delete") : qsTr("Delete session")
                glyph: "✕"
                onClicked: {
                    if (root.confirmDelete)
                        root.cut(root.session.startedOn, root.session.endedOn);
                    else
                        root.confirmDelete = true;
                }
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButtonBase {
                objectName: "sessionCutButton"
                text: qsTr("Remove this time")
                onClicked: root.cut(root.cutFrom, root.cutTo)
            }
        }

        Text {
            objectName: "sessionError"
            Layout.fillWidth: true
            visible: root.error !== ""
            text: root.error
            color: Theme.warning
            font.pixelSize: Theme.fontLabel
            wrapMode: Text.WordWrap
        }
    }
}

import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property int contentPadding: Theme.pad
    property int contentSpacing: Theme.gap

    property alias headerContent: headerExtras.data

    default property alias body: bodyColumn.data

    implicitHeight: layout.implicitHeight + contentPadding * 2

    color: Theme.surface
    radius: Theme.radius
    border.width: 1
    border.color: Theme.border

    ColumnLayout {
        id: layout

        anchors.fill: parent
        anchors.margins: root.contentPadding
        spacing: root.title !== "" ? Theme.gap + 2 : 0

        RowLayout {
            Layout.fillWidth: true
            visible: root.title !== ""
            spacing: Theme.gap

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    text: root.title
                    color: Theme.ink
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    visible: root.subtitle !== ""
                    text: root.subtitle
                    color: Theme.inkFaint
                    font.pixelSize: Theme.fontLabel
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                id: headerExtras
                spacing: Theme.gapTight
            }
        }

        ColumnLayout {
            id: bodyColumn
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: root.contentSpacing
        }
    }
}

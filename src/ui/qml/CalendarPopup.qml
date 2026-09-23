import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: root

    property date selected: new Date()

    // Re-stamped on every show. Evaluated once at page load, it left an
    // instance that had been alive since before midnight comparing against
    // yesterday's clock time, which greyed out today and made it unreachable
    // without a restart.
    property date shownNow: new Date()

    // Latest selectable day, compared by day rather than by timestamp.
    property date maximum: shownNow

    // Extra days past `maximum` that stay selectable, for an exclusive range
    // end that has to be able to reach tomorrow.
    property int maximumDayOffset: 0

    readonly property date maximumDay: new Date(maximum.getFullYear(), maximum.getMonth(), maximum.getDate() + maximumDayOffset)

    signal picked(date value)

    property int shownYear: selected.getFullYear()
    property int shownMonth: selected.getMonth()

    readonly property int leadingBlanks: (new Date(shownYear, shownMonth, 1).getDay() + 6) % 7
    readonly property int daysInMonth: new Date(shownYear, shownMonth + 1, 0).getDate()

    padding: Theme.gap
    modal: false
    focus: true
    implicitWidth: 268

    onAboutToShow: {
        shownNow = new Date();
        shownYear = selected.getFullYear();
        shownMonth = selected.getMonth();
    }

    background: Rectangle {
        color: Theme.surfaceRaised
        radius: Theme.radius
        border.width: 1
        border.color: Theme.border
    }

    function shiftMonth(delta) {
        const moved = new Date(shownYear, shownMonth + delta, 1);
        shownYear = moved.getFullYear();
        shownMonth = moved.getMonth();
    }

    function sameDay(a, b) {
        return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth() && a.getDate() === b.getDate();
    }

    contentItem: ColumnLayout {
        spacing: Theme.gap

        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            ToolButtonBase {
                glyph: "‹"
                compact: true
                onClicked: root.shiftMonth(-1)
            }

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: Qt.locale(settingsController.resolvedLanguage).standaloneMonthName(root.shownMonth) + " " + root.shownYear
                color: Theme.ink
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }

            ToolButtonBase {
                glyph: "›"
                compact: true
                onClicked: root.shiftMonth(1)
            }
        }

        Grid {
            Layout.fillWidth: true
            columns: 7
            rowSpacing: 2
            columnSpacing: 2

            Repeater {
                model: 7

                Item {
                    required property int index

                    implicitWidth: 34
                    implicitHeight: 20

                    Text {
                        anchors.centerIn: parent
                        text: Qt.locale(settingsController.resolvedLanguage).dayName((index + 1) % 7, Locale.NarrowFormat)
                        color: Theme.inkFaint
                        font.pixelSize: Theme.fontMicro
                    }
                }
            }

            Repeater {
                model: root.leadingBlanks + root.daysInMonth

                Item {
                    id: cell

                    required property int index

                    readonly property int dayNumber: index - root.leadingBlanks + 1
                    readonly property bool inMonth: dayNumber >= 1
                    readonly property date cellDate: new Date(root.shownYear, root.shownMonth, Math.max(1, dayNumber))
                    readonly property bool isSelected: inMonth && root.sameDay(cellDate, root.selected)
                    readonly property bool isToday: inMonth && root.sameDay(cellDate, root.shownNow)
                    readonly property bool pickable: inMonth && cellDate <= root.maximumDay

                    implicitWidth: 34
                    implicitHeight: 30

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 1
                        radius: Theme.radiusSmall
                        visible: cell.inMonth
                        color: cell.isSelected ? Theme.accent : mouse.containsMouse ? Theme.hover : "transparent"
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: cell.inMonth
                        text: cell.dayNumber
                        color: cell.isSelected ? "#ffffff" : Theme.ink
                        opacity: cell.pickable || cell.isSelected ? 1 : 0.4
                        font.pixelSize: Theme.fontLabel
                        font.weight: cell.isToday ? Font.DemiBold : Font.Normal
                    }

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 3
                        visible: cell.isToday && !cell.isSelected
                        width: 4
                        height: 4
                        radius: 2
                        color: Theme.accent
                    }

                    MouseArea {
                        id: mouse
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: cell.pickable
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.picked(cell.cellDate);
                            root.close();
                        }
                    }
                }
            }
        }
    }
}

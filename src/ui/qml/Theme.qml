pragma Singleton

import QtCore
import QtQuick

QtObject {
    id: theme

    property alias dark: prefs.dark

    readonly property Settings prefs: Settings {
        id: prefs
        category: "appearance"
        property bool dark: true
    }

    function toggleDark() {
        theme.dark = !theme.dark;
    }

    readonly property color ink: dark ? "#ffffff" : "#0b0b0b"
    readonly property color inkMuted: dark ? "#c3c2b7" : "#52514e"
    readonly property color inkFaint: "#898781"

    readonly property color plane: dark ? "#0d0d0d" : "#f4f4f2"
    readonly property color surface: dark ? "#1a1a19" : "#fcfcfb"
    readonly property color surfaceSunken: dark ? "#111110" : "#f0efec"
    readonly property color surfaceRaised: dark ? "#232322" : "#ffffff"
    readonly property color hover: dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.04)
    readonly property color pressed: dark ? Qt.rgba(1, 1, 1, 0.11) : Qt.rgba(0, 0, 0, 0.08)
    readonly property color selected: dark ? Qt.rgba(0.224, 0.529, 0.898, 0.20) : Qt.rgba(0.165, 0.471, 0.839, 0.12)

    readonly property color border: dark ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0.043, 0.043, 0.043, 0.10)
    readonly property color grid: dark ? "#2c2c2a" : "#e1e0d9"
    readonly property color axis: dark ? "#383835" : "#c3c2b7"
    readonly property color track: dark ? "#26262a" : "#e8e7e2"
    readonly property color tooltip: dark ? "#2c2c2a" : "#1a1a19"
    readonly property color tooltipInk: "#ffffff"

    readonly property color good: "#0ca30c"
    readonly property color warning: "#fab219"

    readonly property var seriesLight: ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#4a3aa7", "#e34948"]
    readonly property var seriesDark: ["#3987e5", "#d95926", "#199e70", "#c98500", "#d55181", "#008300", "#9085e9", "#e66767"]
    readonly property color seriesNeutral: "#898781"

    readonly property color accent: series(0)

    function series(slot) {
        if (slot === undefined || slot < 0 || slot >= seriesLight.length)
            return seriesNeutral;
        return dark ? seriesDark[slot] : seriesLight[slot];
    }

    readonly property int radius: 10
    readonly property int radiusSmall: 6
    readonly property int radiusPill: 999

    readonly property int gapTight: 6
    readonly property int gap: 12
    readonly property int gapLoose: 20
    readonly property int pad: 18

    readonly property int rowHeight: 34
    readonly property int controlHeight: 32

    readonly property int fontHero: 42
    readonly property int fontDisplay: 26
    readonly property int fontTitle: 15
    readonly property int fontBody: 13
    readonly property int fontLabel: 12
    readonly property int fontMicro: 11

    function niceCeilSeconds(seconds) {
        if (seconds <= 0)
            return 3600;
        const steps = [300, 600, 900, 1800, 3600, 7200, 10800, 14400, 21600, 28800, 43200, 57600, 86400];
        for (let i = 0; i < steps.length; ++i) {
            if (seconds <= steps[i])
                return steps[i];
        }
        return Math.ceil(seconds / 86400) * 86400;
    }
}

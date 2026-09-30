pragma Singleton
import QtQuick

// Design tokens modelled on the reMarkable UI: paper white, ink black,
// one light grey for rules, hairline separators, generous touch targets.
QtObject {
    readonly property color paper: "white"
    readonly property color ink: "black"
    readonly property color muted: "#595959"
    readonly property color faint: "#9a9a9a"
    readonly property color rule: "#c4c4c4"
    readonly property color selection: "#e6e6e6"
    readonly property string font: "Noto Sans"
}

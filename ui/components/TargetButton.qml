import QtQuick
import QtQuick.Controls

/**
 * Переиспользуемая кнопка для управления целями
 */
Button {
    id: control

    property string buttonType: "default" // "up", "down", "delete", "default"

    width: 30
    height: 25

    contentItem: Text {
        text: control.text
        font.pointSize: 12
        color: {
            if (!control.enabled) return "gray"
            return control.buttonType === "delete" ? "red" : "white"
        }
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        color: {
            if (!control.enabled) return "#222222"
            return control.buttonType === "delete" ? "#660000" : "#444444"
        }
        radius: 4
        border.color: control.buttonType === "delete" ? "red" : "#666666"
        border.width: 1

        // Hover effect
        Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: control.hovered && control.enabled ? 0.1 : 0
            radius: parent.radius

            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }
    }
}
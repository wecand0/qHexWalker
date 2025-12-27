import QtQuick
import QtQuick.Controls

/**
 * Компонент для отображения подсказки о горячей клавише
 */
Rectangle {
    id: root

    property string hintText: ""
    property string textColor: "white"
    property alias fontSize: hintTextItem.font.pointSize

    implicitWidth: hintTextItem.width + 20
    implicitHeight: hintTextItem.height + 8

    border.color: "#66FFFFFF"
    border.width: 1
    color: "black"
    opacity: 0.9
    radius: 7

    Behavior on opacity {
        NumberAnimation { duration: 200 }
    }

    Text {
        id: hintTextItem
        anchors.centerIn: parent
        font.pointSize: 20
        color: root.textColor
        text: root.hintText
    }

    // Pulse animation on hover
    states: State {
        name: "hovered"
        when: mouseArea.containsMouse
        PropertyChanges {
            target: root
            scale: 1.05
        }
    }

    transitions: Transition {
        NumberAnimation {
            properties: "scale"
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
    }
}
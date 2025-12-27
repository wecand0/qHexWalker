import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Элемент списка целей (Target)
 */
Rectangle {
    id: listItem

    // Properties from model (fallback to safe defaults if not available)
    property int order: model && model.order !== undefined ? model.order : 0
    property var h3Index: model && model.h3Index !== undefined ? model.h3Index : 0
    property int res: model && model.res !== undefined ? model.res : 0
    property int zoom: model && model.zoom !== undefined ? model.zoom : 0
    property var coordinate: model && model.coordinate !== undefined ? model.coordinate : null

    // Properties from ListView
    property int index: model && model.index !== undefined ? model.index : 0
    readonly property int count: ListView.view ? ListView.view.count : 0

    signal moveUp()
    signal moveDown()
    signal deleteItem()
    signal itemClicked()

    implicitHeight: itemColumn.height + 16
    implicitWidth: 200

    Component.onCompleted: {
        //console.log("TargetListItem created:", index, "order:", order, "h3Index:", h3Index)
    }

    border.color: "#555555"
    border.width: 1
    color: itemMouseArea.containsMouse ? "#496e93" : "#242B33"
    radius: 4

    Behavior on color {
        ColorAnimation { duration: 150 }
    }

    Behavior on opacity {
        NumberAnimation { duration: 200 }
    }

    MouseArea {
        id: itemMouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: listItem.itemClicked()
    }

    Column {
        id: itemColumn
        spacing: 4
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 8
        }

        // Header with order number and control buttons
        Row {
            spacing: 4
            width: parent.width

            Text {
                color: "#FFD700"
                font.bold: true
                text: "#" + listItem.order
                width: 40
                anchors.verticalCenter: parent.verticalCenter
            }

            TargetButton {
                buttonType: "up"
                enabled: listItem.index > 0
                text: "▲"
                onClicked: listItem.moveUp()
            }

            TargetButton {
                buttonType: "down"
                enabled: listItem.index < listItem.count - 1
                text: "▼"
                onClicked: listItem.moveDown()
            }

            TargetButton {
                buttonType: "delete"
                text: "✕"
                onClicked: listItem.deleteItem()
            }
        }

        // H3 Index
        Text {
            color: "lightblue"
            font.pixelSize: 11
            text: "H3: 0x" + listItem.h3Index.toString(16)
            width: parent.width
            wrapMode: Text.WrapAnywhere
        }

        // Resolution and Zoom
        Row {
            spacing: 10

            Text {
                color: "orange"
                font.pixelSize: 11
                text: "Res: " + listItem.res
            }

            Text {
                color: "orange"
                font.pixelSize: 11
                text: "Zoom: " + listItem.zoom
            }
        }
    }
}
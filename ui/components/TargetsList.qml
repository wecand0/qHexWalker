import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Панель со списком целей (Targets)
 */
Rectangle {
    id: root

    // Signals
    signal targetClicked(var coordinate, var zoom)

    // Note: targetsModel and h3Model are accessed directly from context
    // No property declarations to avoid shadowing context properties

    Component.onCompleted: {
        // console.log("TargetsList: Component completed")
        // console.log("TargetsList: targetsModel =", targetsModel)
        // console.log("TargetsList: h3Model =", h3Model)
    }

    color: "#1E252B"
    opacity: 0.85
    radius: 16

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 8

        // Header
        Text {
            Layout.fillWidth: true
            color: "white"
            font.bold: true
            font.pixelSize: 22
            text: "H3 Targets List"
        }

        Rectangle {
            color: "#444444"
            Layout.fillWidth: true
            height: 1
        }

        // List of targets
        ScrollView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff
            clip: true

            ListView {
                id: coordinateListView

                model: targetsModel
                spacing: 4
                focus: true
                keyNavigationEnabled: true
                highlightFollowsCurrentItem: true

                // Debug
                Component.onCompleted: {
                    // console.log("ListView initialized. Model:", targetsModel)
                    // console.log("Initial count:", count)
                }

                onCountChanged: {
                    //console.log("ListView count changed:", count)
                }

                // Highlight
                highlight: Rectangle {
                    color: "#4169E1"
                    radius: 4
                    opacity: 0.3
                }

                // Keyboard navigation
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Up && currentIndex > 0) {
                        targetsModel.move(currentIndex, currentIndex - 1)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Down && currentIndex < count - 1) {
                        targetsModel.move(currentIndex, currentIndex + 1)
                        event.accepted = true
                    }
                }

                // Animations
                add: Transition {
                    NumberAnimation {
                        properties: "opacity"
                        from: 0
                        to: 1
                        duration: 300
                    }
                }

                addDisplaced: Transition {
                    NumberAnimation { properties: "x, y"; duration: 300 }
                }

                moveDisplaced: Transition {
                    NumberAnimation { properties: "x, y"; duration: 300 }
                }

                remove: Transition {
                    ParallelAnimation {
                        NumberAnimation { property: "opacity"; to: 0; duration: 300 }
                        NumberAnimation { property: "scale"; to: 0.8; duration: 300 }
                    }
                }

                removeDisplaced: Transition {
                    NumberAnimation { properties: "x, y"; duration: 300 }
                }

                displaced: Transition {
                    NumberAnimation { properties: "x, y"; duration: 300 }
                }

                move: Transition {
                    NumberAnimation { properties: "x, y"; duration: 300 }
                }

                // Delegate
                delegate: TargetListItem {
                    width: coordinateListView.width

                    // Properties are automatically bound from model roles
                    // No need to explicitly set them with Qt 6 required properties

                    onMoveUp: {
                        targetsModel.move(index, index - 1)
                    }

                    onMoveDown: {
                        targetsModel.move(index, index + 1)
                    }

                    onDeleteItem: {
                        let cellsNumber = targetsModel.remove(index)
                        if (cellsNumber === 0) {
                            h3Model.clearAllCells()
                        }
                    }

                    onItemClicked: {
                        coordinateListView.currentIndex = index
                        coordinateListView.forceActiveFocus()
                        root.targetClicked(coordinate, zoom)
                    }
                }
            }
        }
    }
}
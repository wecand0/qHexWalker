import QtQuick
import QtQuick.Window
import QtLocation
import QtPositioning
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window

    // Тёмная тема
    Material.theme: Material.Dark
    Material.accent: Material.Teal
    Material.primary: Material.BlueGrey

    property var coordinate: QtPositioning.coordinate(0.0, 0.0)
    property var zoomTarget: 0
    property var visibleBounds: ({
            north: 0,
            south: 0,
            east: 0,
            west: 0
        })

    function getRandomColor() {
        var r = Math.floor(Math.random() * 256);
        var g = Math.floor(Math.random() * 256);
        var b = Math.floor(Math.random() * 256);
        return Qt.rgba(r / 255, g / 255, b / 255, 0.6);
    }

    // Функция проверки вхождения координаты в границы
    function isCoordinateInBounds(coord, bounds) {
        if (!coord || !coord.isValid)
            return false;

        var lat = coord.latitude;
        var lon = coord.longitude;

        // Нормализация долготы
        while (lon > 180)
            lon -= 360;
        while (lon < -180)
            lon += 360;

        // Проверка широты
        if (lat < bounds.south || lat > bounds.north)
            return false;

        // Проверка долготы с учетом перехода через 180-й меридиан
        if (bounds.east >= bounds.west) {
            // Обычный случай
            return lon >= bounds.west && lon <= bounds.east;
        } else {
            // Переход через 180-й меридиан
            return lon >= bounds.west || lon <= bounds.east;
        }
    }

    // Функция проверки вхождения полигона в границы
    function isPolygonVisible(polygonPath, bounds) {
        if (!polygonPath || polygonPath.length === 0)
            return false;
        for (var i = 0; i < polygonPath.length; i++) {
            if (isCoordinateInBounds(polygonPath[0], bounds))
                return false;
        }
        return true;
    }

    // Функция обновления порядка после перемещения
    function updateOrder() {
        for (var i = 0; i < targetsModel.count; i++) {
            targetsModel.setProperty(i, "order", i + 1);
        }
    }

    height: Qt.platform.os === "android" ? Screen.height : Screen.height
    visible: true
    width: Qt.platform.os === "android" ? Screen.width : Screen.width

    Component.onCompleted: {}

    Plugin {
        id: mapPlugin

        name: "maplibre"

        parameters: [
            PluginParameter {
                name: "maplibre.map.styles"
                value: mapProvider ? mapProvider.url : "https://demotiles.maplibre.org/style.json"
            }
        ]
    }
    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // Панель со списком координат
        Rectangle {
            id: paths

            implicitWidth: Screen.width * 0.15
            SplitView.maximumWidth:  Screen.width * 0.2
            SplitView.minimumWidth:  Screen.width * 0.1
            color: "#1E252B"
            opacity: 0.85
            radius: 16

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 8

                // Заголовок
                Text {
                    Layout.fillWidth: true
                    color: "white"
                    font.bold: true
                    font.pixelSize: 22
                    text: "H3 targets list"
                }
                Rectangle {
                    color: "#444444"
                    Layout.fillWidth: true
                    height: 1
                }
                // Список координат
                ScrollView {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                    //clip: true

                    ListView {
                        id: coordinateListView

                        model: targetsModel
                        spacing: 4

                        // Анимации перемещения (красиво)
                        move: Transition {
                            NumberAnimation { properties: "x,y"; duration: 300 }
                        }
                        moveDisplaced: Transition {
                            NumberAnimation { properties: "x,y"; duration: 300 }
                        }

                        delegate: Rectangle {
                            id: listItem
                            border.color: "#555555"
                            border.width: 1
                            color: itemMouseArea.containsMouse ? "#496e93" : "#242B33"
                            height: itemColumn.height + 16
                            radius: 4
                            width: coordinateListView.width

                            Behavior on color { ColorAnimation { duration: 150 } }

                            MouseArea {
                                id: itemMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    centerAnimation.to = model.coordinate
                                    zoomAnimation.to = model.zoom
                                    centerAnimation.start()
                                    zoomAnimation.start()
                                }
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

                                Row {
                                    spacing: 4
                                    width: parent.width

                                    Text {
                                        color: "#FFD700"
                                        font.bold: true
                                        text: "#" + model.order
                                        width: 40
                                    }

                                    // Кнопка вверх
                                    Button {
                                        width: 30
                                        height: 25
                                        enabled: index > 0
                                        text: "▲"

                                        contentItem: Text {
                                            text: parent.text
                                            font.pointSize: 12
                                            color: parent.enabled ? "white" : "gray"
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        background: Rectangle {
                                            color: parent.enabled ? "#444444" : "#222222"
                                            radius: 4
                                            border.color: "#666666"
                                        }

                                        onClicked: targetsModel.move(index, index - 1)
                                    }

                                    // Кнопка вниз
                                    Button {
                                        width: 30
                                        height: 25
                                        enabled: index < coordinateListView.count - 1
                                        text: "▼"

                                        contentItem: Text {
                                            text: parent.text
                                            font.pointSize: 12
                                            color: parent.enabled ? "white" : "gray"
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        background: Rectangle {
                                            color: parent.enabled ? "#444444" : "#222222"
                                            radius: 4
                                            border.color: "#666666"
                                        }

                                        onClicked: targetsModel.move(index, index + 1)
                                    }

                                    // Кнопка удаления
                                    Button {
                                        width: 30
                                        height: 25
                                        text: "✕"

                                        contentItem: Text {
                                            text: parent.text
                                            font.pointSize: 12
                                            color: "red"
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        background: Rectangle {
                                            color: "#660000"
                                            radius: 4
                                            border.color: "red"
                                        }

                                        onClicked: {
                                            let cellsNumber = targetsModel.remove(index)
                                            if(cellsNumber === 0){
                                                h3Model.clearAllCells();
                                            }
                                        }
                                    }
                                }


                                // H3 индекс
                                Text {
                                    color: "lightblue"
                                    font.pixelSize: 11
                                    text: "H3: 0x" + model.h3Index.toString(16)
                                    width: parent.width
                                    wrapMode: Text.WrapAnywhere
                                }

                                // Координаты
                                // Text {
                                //     color: "lightgreen"
                                //     font.pixelSize: 11
                                //     text: "Lat: " + model.coordinate.latitude.toFixed(3) + " Lng: " + model.coordinate.longitude.toFixed(3)
                                //     width: parent.width
                                // }

                                // Разрешение и зум
                                Row {
                                    spacing: 10

                                    Text {
                                        color: "orange"
                                        font.pixelSize: 11
                                        text: "Res: " + model.res
                                    }
                                    Text {
                                        color: "orange"
                                        font.pixelSize: 11
                                        text: "Zoom: " + model.zoom
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        Map {
            id: map

            PropertyAnimation {
                id: centerAnimation
                target: map
                property: "center"
                to: QtPositioning.coordinate(55.0, 55.0) // The desired end zoom level
                duration: 500 // Animation duration in milliseconds
                easing.type:Easing.OutCubic // Optional: for smoother animation
            }
            PropertyAnimation {
                id: zoomAnimation
                target: map
                property: "zoomLevel"
                to: 10 // The desired end zoom level
                duration: 500 // Animation duration in milliseconds
                easing.type: Easing.OutCubic // Optional: for smoother animation
            }
            function normalizeLon(lon) {
                var x = lon;
                while (x > 180)
                    x -= 360;
                while (x < -180)
                    x += 360;
                return x;
            }
            function updateVisibleBounds() {
                if (width <= 0 || height <= 0)
                    return;

                const pNW = Qt.point(0, 0);
                const pNE = Qt.point(width, 0);
                const pSW = Qt.point(0, height);
                const pSE = Qt.point(width, height);

                const cNW = toCoordinate(pNW);
                const cNE = toCoordinate(pNE);
                const cSW = toCoordinate(pSW);
                const cSE = toCoordinate(pSE);

                var lats = [cNW.latitude, cNE.latitude, cSW.latitude, cSE.latitude];
                var lons = [normalizeLon(cNW.longitude), normalizeLon(cNE.longitude), normalizeLon(cSW.longitude), normalizeLon(cSE.longitude)];

                var north = Math.max(lats[0], lats[1], lats[2], lats[3]);
                var south = Math.min(lats[0], lats[1], lats[2], lats[3]);

                var west = Math.min(lons[0], lons[1], lons[2], lons[3]);
                var east = Math.max(lons[0], lons[1], lons[2], lons[3]);

                visibleBounds = {
                    north: north,
                    south: south,
                    east: east,
                    west: west
                };

            //debugBounds.text = " Window: " + window.width + "x" + window.height + " | Map: " + width + "x" + height + " BBOX: N " + north.toFixed(1) + " S " + south.toFixed(1) + " E " + east.toFixed(1) + " W " + west.toFixed(1);
            }

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredHeight: parent.height
            center: QtPositioning.coordinate(55.0, 55.0)
            maximumFieldOfView: map.maximumFieldOfView
            maximumZoomLevel: 15
            minimumFieldOfView: map.minimumFieldOfView
            minimumZoomLevel: 3
            objectName: "map"
            plugin: mapPlugin
            zoomLevel: 3

            Component.onCompleted: {
                updateVisibleBounds();
            }
            onBearingChanged: updateVisibleBounds()
            onCenterChanged: updateVisibleBounds()
            onHeightChanged: updateVisibleBounds()
            onMapReadyChanged: updateVisibleBounds()
            onTiltChanged: {
                if (map.tilt > map.maximumTilt * 0.9) {
                    map.tilt = map.maximumTilt * 0.9;
                }
                updateVisibleBounds();
            }
            onWidthChanged: updateVisibleBounds()
            onZoomLevelChanged: {
                if (map.zoomLevel <= 3) {
                    map.zoomLevel = 3;
                }
                updateVisibleBounds();
            }

            // Connections для безопасной очистки модели
            Connections {
                function onClearingFinished() {
                    //console.log("Map: Clearing finished - recreating MapItemView");
                    // Восстанавливаем MapItemView
                    if (cells) {
                        cells.model = h3Model;
                        cells.visible = true;
                    }
                }
                function onClearingStarted() {
                    //console.log("Map: Clearing started - destroying MapItemView");
                    // Полностью уничтожаем MapItemView
                    if (cells) {
                        cells.visible = false;
                        cells.model = null;
                    }
                }

                target: h3Model
            }
            Connections {
                function onClearingFinished() {
                    //console.log("Map: Clearing finished - recreating MapItemView");
                    // Восстанавливаем MapItemView
                    if(targetCells) {
                        targetCells.model = targetsModel;
                        targetCells.visible = true;
                    }
                }
                function onClearingStarted() {
                    //console.log("Map: Clearing started - destroying MapItemView");
                    // Полностью уничтожаем MapItemView
                    if(targetCells) {
                        targetCells.visible = false;
                        targetCells.model = null;
                    }
                }

                target: targetsModel
            }
            DragHandler {
                id: drag

                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                target: null

                onTranslationChanged: delta => map.pan(-delta.x, -delta.y)
            }
            MouseArea {
                id: mapMouseArea

                property var currentCoordinate: map.toCoordinate(Qt.point(mouseX, mouseY))
                property real prevY: -1

                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                cursorShape: Qt.CrossCursor

                hoverEnabled: true

                onClicked: event => {
                    if (event.button === Qt.LeftButton) {
                        currentCoordinate = map.toCoordinate(Qt.point(event.x, event.y));
                    }
                }
                onDoubleClicked: event => {
                    let mouseGeoPos = map.toCoordinate(Qt.point(event.x, event.y));
                    let preZoomPoint = map.fromCoordinate(mouseGeoPos, false);
                    map.zoomLevel += (event.button === Qt.LeftButton) ? 1 : -1;
                    let postZoomPoint = map.fromCoordinate(mouseGeoPos, false);
                    let dx = postZoomPoint.x - preZoomPoint.x;
                    let dy = postZoomPoint.y - preZoomPoint.y;
                    map.center = map.toCoordinate(Qt.point(map.width / 2 + dx, map.height / 2 + dy));
                }
                onPositionChanged: event => {
                    currentCoordinate = map.toCoordinate(Qt.point(event.x, event.y));
                }
                onWheel: event => {
                    let mouseGeoPos = map.toCoordinate(Qt.point(event.x, event.y));
                    let preZoomPoint = map.fromCoordinate(mouseGeoPos, false);
                    map.zoomLevel += (event.angleDelta.y > 0) ? 0.5 : -0.5;
                    let postZoomPoint = map.fromCoordinate(mouseGeoPos, false);
                    let dx = postZoomPoint.x - preZoomPoint.x;
                    let dy = postZoomPoint.y - preZoomPoint.y;
                    map.center = map.toCoordinate(Qt.point(map.width / 2 + dx, map.height / 2 + dy));
                }
            }
            Shortcut {
                sequence: "z"
                onActivated: {
                    centerAnimation.to = mapMouseArea.currentCoordinate
                    zoomAnimation.to = 3
                    centerAnimation.start()
                    zoomAnimation.start()
                }
            }
            Shortcut {
                sequence: "a"
                onActivated: {
                    targetsModel.requestCell(map.zoomLevel.toFixed(1), mapMouseArea.currentCoordinate)
                }
            }
            Rectangle {
                id: addTarget

                anchors.margins: 8
                anchors.top: parent.top
                anchors.left: parent.left
                border.color: "#66FFFFFF"
                border.width: 1
                color: "black"
                height: addTargetTxt.height
                opacity: 1
                radius: 7
                width: addTargetTxt.width
                z: 1

                Text {
                    id: addTargetTxt
                    font.pointSize: 20
                    color: "green"
                    text: " Press 'a' to add a target "
                }
            }
            Shortcut {
                sequence: "c"
                onActivated: {
                    targetsModel.compute();
                }
            }
            Rectangle {
                id: computePath

                anchors.margins: 8
                anchors.top: addTarget.bottom
                anchors.left: parent.left
                border.color: "#66FFFFFF"
                border.width: 1
                color: "black"
                height: computePathTxt.height
                opacity: 1
                radius: 7
                width: computePathTxt.width
                z: 1

                Text {
                    id: computePathTxt
                    font.pointSize: 20
                    color: "yellow"
                    text: " Press 'c' to compute  "
                }
            }
            Shortcut {
                sequence: "r"
                onActivated: {
                    targetsModel.clearAllCells();
                    h3Model.clearAllCells();
                }
            }
            Rectangle {
                id: clearAll

                anchors.margins: 8
                anchors.top: computePath.bottom
                anchors.left: parent.left
                border.color: "#66FFFFFF"
                border.width: 1
                color: "black"
                height: clearAllTxt.height
                opacity: 1
                radius: 7
                width: clearAllTxt.width
                z: 1

                Text {
                    id: clearAllTxt
                    font.pointSize: 20
                    color: "red"
                    text: " Press 'r' to clear all cells "
                }
            }
            MapItemView {
                id: targetCells
                model: targetsModel ? targetsModel : null
                visible: true

                delegate: Component {
                    id: cellDelegate
                    MapItemGroup {
                        MapPolygon {
                            id: cellLine
                            autoFadeIn: false
                            border.color: "black"
                            border.width: 1
                            color: model ? model.color : "transparent"
                            opacity: model ? model.res * 0.1 : 1
                            path: model ? model.path : []
                            referenceSurface: QtLocation.ReferenceSurface.Globe
                            visible: true
                            z: model ? model.res : 2
                            MouseArea {
                                id: mouseID
                                anchors.fill: parent
                                hoverEnabled: true
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                cursorShape: Qt.CrossCursor

                                onClicked: event => {
                                    if (event.button === Qt.LeftButton) {
                                        centerAnimation.to = model.coordinate
                                        zoomAnimation.to = model.zoom
                                        centerAnimation.start()
                                        zoomAnimation.start()
                                    }
                                    if (event.button === Qt.RightButton) {
                                        targetsModel.remove(index)
                                    }
                                }
                            }
                        }

                        MapQuickItem {
                            coordinate: model ? model.coordinate : QtPositioning.coordinate()
                            anchorPoint: Qt.point(sourceItem.width / 2, sourceItem.height / 2)
                            z: model ? model.res + 1 : 3

                            sourceItem: Rectangle {
                                width: textMetrics.width + 10
                                height: textMetrics.height + 6
                                color: "white"
                                border.color: "black"
                                opacity: 1
                                radius: 7

                                Text {
                                    id: cellText
                                    anchors.centerIn: parent
                                    text: model ? model.order: 0
                                    font.pixelSize: 12
                                    color: "black"
                                }

                                TextMetrics {
                                    id: textMetrics
                                    font: cellText.font
                                    text: cellText.text
                                }
                                MouseArea {
                                    id: mouseRID
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    cursorShape: Qt.CrossCursor

                                    onClicked: event => {
                                        if (event.button === Qt.LeftButton) {
                                            centerAnimation.to = model.coordinate
                                            zoomAnimation.to = model.zoom
                                            centerAnimation.start()
                                            zoomAnimation.start()
                                        }
                                        if (event.button === Qt.RightButton) {
                                            targetsModel.remove(index)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            MapItemView {
                id: cells

                model: h3Model ? h3Model : null
                visible: true

                delegate: MapPolygon {
                    id: cellLine

                    autoFadeIn: false
                    border.color: "black"
                    border.width: 1
                    color: model ? model.color : "transparent"
                    opacity: model ? model.res * 0.1 : 1
                    path: model ? model.path : []
                    referenceSurface: QtLocation.ReferenceSurface.Globe
                    visible: true
                    z: model ? model.res : 2
                }
            }
            Rectangle {
                id: copyRight

                anchors.margins: 8
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                border.color: "#66FFFFFF"
                border.width: 1
                color: "darkslategray"
                height: copyRightTxt.height * 1.1
                opacity: 0.85
                radius: 7
                width: copyRightTxt.width
                z: 1

                Text {
                    id: copyRightTxt

                    color: "white"
                    text: " © MapTiler © OpenStreetMap contributors "
                }
            }
            Rectangle {
                id: latLngTxtRect

                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 8
                border.color: "#66FFFFFF"
                border.width: 1
                color: "darkslategray"
                height: latLngTxt.height * 1.1
                opacity: 0.85
                radius: 7
                width: latLngTxt.width + 10
                z: 1

                Text {
                    id: latLngTxt

                    color: "white"
                    text: " Zoom: %1".arg(map.zoomLevel.toFixed(1))
                }
            }
            // Rectangle {
            //     id: debugOverlay
            //
            //     anchors.bottom: parent.bottom
            //     anchors.margins: 8
            //     anchors.right: parent.right
            //     border.color: "#66FFFFFF"
            //     border.width: 1
            //     color: "darkslategray"
            //     height: debugBounds.height * 1.1
            //     opacity: 0.85
            //     radius: 7
            //     width: debugBounds.width + 10
            //     z: 1
            //
            //     Text {
            //         id: debugBounds
            //
            //         color: "white"
            //         text: ""
            //     }
            // }
        }
    }

    // Модель для хранения координат
    ListModel {
        id: coordinateListModel

    }
}
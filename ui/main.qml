import QtQuick
import QtQuick.Window
import QtLocation
import QtPositioning
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window

    property var coordinate: QtPositioning.coordinate(0.0, 0.0)
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
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Панель со списком координат
        Rectangle {
            id: paths

            Layout.fillHeight: true
            Layout.minimumWidth: 250
            Layout.preferredWidth: Screen.width * 0.2
            border.color: "#66FFFFFF"
            border.width: 1
            color: "#2A2A2A"
            opacity: 0.85
            radius: 7
            z: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                // Заголовок
                Text {
                    Layout.fillWidth: true
                    color: "white"
                    font.bold: true
                    font.pixelSize: 16
                    text: "H3 path list"
                }
                Rectangle {
                    Layout.fillWidth: true
                    color: "#66FFFFFF"
                    height: 1
                }

                // Кнопка добавления текущей координаты
                Button {
                    Layout.fillWidth: true
                    text: "Add Position"

                    contentItem: Text {
                        color: "white" // Set your desired color here
                        font.pointSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        text: parent.text // Referencing the button's text
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        targetsModel.requestCell(map.zoomLevel.toFixed(1), mapMouseArea.currentCoordinate)
                    }
                }

                // Список координат
                ScrollView {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    clip: true

                    ListView {
                        id: coordinateListView

                        model: targetsModel
                        spacing: 4

                        delegate: Rectangle {
                            id: listItem

                            border.color: "#555555"
                            border.width: 1
                            color: itemMouseArea.containsMouse ? "#404040" : "#333333"
                            height: itemColumn.height + 16
                            radius: 4
                            width: coordinateListView.width

                            MouseArea {
                                id: itemMouseArea

                                anchors.fill: parent
                                hoverEnabled: true

                                onClicked: {
                                    map.center = model.coordinate;
                                    map.zoomLevel = parseFloat(model.zoom);
                                }
                            }
                            Column {
                                id: itemColumn

                                spacing: 4

                                anchors {
                                    left: parent.left
                                    margins: 8
                                    right: parent.right
                                    top: parent.top
                                }

                                // Порядковый номер и кнопки управления
                                Row {
                                    spacing: 4
                                    width: parent.width

                                    Text {
                                        color: "#FFD700"
                                        font.bold: true
                                        text: "#" + model.order
                                        width: 40
                                    }

                                    // Кнопки перемещения
                                    Button {
                                        id: upButton
                                        enabled: index > 0
                                        height: 25
                                        text: "▲"
                                        width: 30

                                        contentItem: Text {
                                            color: upButton.enabled ? "white" : "gray"
                                            font.pointSize: 12
                                            horizontalAlignment: Text.AlignHCenter
                                            text: parent.text // Referencing the button's text
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        onClicked: {
                                            if (index > 0) {
                                                coordinateListModel.move(index, index - 1, 1);
                                                updateOrder();
                                            }
                                        }
                                    }
                                    Button {
                                        id: downButton

                                        enabled: index < coordinateListModel.count - 1
                                        height: 25
                                        text: "▼"
                                        width: 30

                                        contentItem: Text {
                                            color: downButton.enabled ? "white" : "gray"
                                            font.pointSize: 12
                                            horizontalAlignment: Text.AlignHCenter
                                            text: parent.text // Referencing the button's text
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        onClicked: {
                                            if (index < coordinateListModel.count - 1) {
                                                coordinateListModel.move(index, index + 1, 1);
                                                updateOrder();
                                            }
                                        }
                                    }
                                    Item {
                                        Layout.fillWidth: true
                                        width: 10
                                    }

                                    // Кнопка удаления
                                    Button {
                                        height: 25
                                        text: "✕"
                                        width: 30

                                        contentItem: Text {
                                            color: "red" // Set your desired color here
                                            font.pointSize: 12
                                            horizontalAlignment: Text.AlignHCenter
                                            text: parent.text // Referencing the button's text
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        onClicked: {
                                            targetsModel.remove(index);
                                            updateOrder();
                                        }
                                    }
                                }

                                // H3 индекс
                                Text {
                                    color: "lightblue"
                                    font.pixelSize: 11
                                    text: "H3: " + model.index
                                    width: parent.width
                                    wrapMode: Text.WrapAnywhere
                                }

                                // Координаты
                                Text {
                                    color: "lightgreen"
                                    font.pixelSize: 11
                                    text: "Lat: " + model.coordinate.latitude.toFixed(6) + " Lng: " + model.coordinate.longitude.toFixed(6)
                                    width: parent.width
                                }

                                // Разрешение и зум
                                Row {
                                    spacing: 10

                                    Text {
                                        color: "orange"
                                        font.pixelSize: 11
                                        text: "Res: " + model.res
                                    }
                                    // Text {
                                    //     color: "orange"
                                    //     font.pixelSize: 11
                                    //     text: "Zoom: " + model.zoom
                                    // }
                                }
                            }
                        }
                    }
                }
                Button {
                    Layout.fillWidth: true
                    text: "Compute"

                    contentItem: Text {
                        color: "green" // Set your desired color here
                        font.pointSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        text: parent.text // Referencing the button's text
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        //coordinateListModel.clear();
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: "Clear All"

                    contentItem: Text {
                        color: "red" // Set your desired color here
                        font.pointSize: 10
                        horizontalAlignment: Text.AlignHCenter
                        text: parent.text // Referencing the button's text
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        coordinateListModel.clear();
                        h3Model.clearAllCells()
                    }
                }

                // Кнопки управления списком

            }
        }
        Map {
            id: map

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

                //hoverEnabled: true

                onClicked: event => {
                    if (event.button === Qt.LeftButton) {
                        h3Model.requestCell(map.zoomLevel, map.toCoordinate(Qt.point(mouseX, mouseY)));
                    }
                    if (event.button === Qt.RightButton) {
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
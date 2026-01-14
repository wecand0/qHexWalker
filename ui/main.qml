import QtQuick
import QtQuick.Window
import QtLocation
import QtPositioning
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window

    // Platform detection - multiple methods for reliability
    readonly property bool isMobile: {
        var os = Qt.platform.os.toLowerCase()
        return os === "android" || os === "ios" || os === "qnx" ||
               (Qt.platform.pluginName && Qt.platform.pluginName.toLowerCase().indexOf("android") >= 0)
    }

    // Constants - adaptive for mobile
    readonly property real sidebarMinWidth: isMobile ? Screen.width * 0.7 : Screen.width * 0.1
    readonly property real sidebarMaxWidth: isMobile ? Screen.width * 0.85 : Screen.width * 0.2
    readonly property real sidebarDefaultWidth: isMobile ? Screen.width * 0.75 : Screen.width * 0.15

    // Тёмная тема
    Material.theme: Material.Dark
    Material.accent: Material.Teal
    Material.primary: Material.BlueGrey

    Component.onCompleted: {
        // console.log("ApplicationWindow loaded")
        // console.log("targetsModel available:", typeof targetsModel)
        // console.log("targetsModel:", targetsModel)
        // console.log("h3Model available:", typeof h3Model)
    }

    property var coordinate: QtPositioning.coordinate(0.0, 0.0)
    property var zoomTarget: 0
    property var isInRingOutOfMaze: false
    property var visibleBounds: ({
            north: 0,
            south: 0,
            east: 0,
            west: 0
        })

    height: Screen.height
    visible: true
    width: Screen.width

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

        // Desktop: Панель со списком целей (refactored to component)
        TargetsList {
            id: targetsList

            visible: !window.isMobile
            implicitWidth: window.sidebarDefaultWidth
            SplitView.maximumWidth: window.sidebarMaxWidth
            SplitView.minimumWidth: window.sidebarMinWidth

            // targetsModel and h3Model are accessed from context directly
            // No need to pass them as properties

            onTargetClicked: function(coordinate, zoom) {
                centerAnimation.to = coordinate
                zoomAnimation.to = zoom
                centerAnimation.start()
                zoomAnimation.start()
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
            // Touch/Mouse drag for panning
            DragHandler {
                id: drag
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.TouchScreen
                target: null
                onTranslationChanged: delta => map.pan(-delta.x, -delta.y)
            }

            // Pinch-to-zoom for touch screens
            PinchHandler {
                // id: pinch
                // target: null
                // onScaleChanged: (delta) => {
                //     map.scale(delta, pinch.centroid.position)
                // }
                // grabPermissions: PointerHandler.TakeOverForbidden
                id: pinch
                target: null
                property real startZoom: 3

                onActiveChanged: {
                    if (active) {
                        startZoom = map.zoomLevel
                    }
                }

                onScaleChanged: {
                    var newZoom = startZoom + Math.log2(scale)
                    newZoom = Math.max(map.minimumZoomLevel, Math.min(map.maximumZoomLevel, newZoom))
                    map.zoomLevel = newZoom
                }
                grabPermissions: PointerHandler.TakeOverForbidden
            }

            // универсальный обработчик (мышь + тач)
            TapHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchScreen
                grabPermissions: PointerHandler.TakeOverForbidden

                onTapped: function(eventPoint) {
                    const p = eventPoint.position
                    currentCoordinate = map.toCoordinate(Qt.point(p.x, p.y))

                    console.log(
                        "Tapped at:",
                        currentCoordinate.latitude,
                        currentCoordinate.longitude
                    )
                }
            }

            MouseArea {
                id: mapMouseArea

                property var currentCoordinate: map.toCoordinate(Qt.point(mouseX, mouseY))
                property real prevY: -1


                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                cursorShape: Qt.CrossCursor

                hoverEnabled: true

                onPressed: (event) => {
                    if (event.button === Qt.LeftButton || event.source === Qt.MouseEventNotSynthesized) {
                        currentCoordinate = map.toCoordinate(Qt.point(event.x, event.y))
                    }
                }
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
            // Keyboard shortcuts
            Shortcut {
                sequence: "a"
                onActivated: {
                    targetsModel.requestCell(map.zoomLevel.toFixed(1), mapMouseArea.currentCoordinate)
                }
            }

            Shortcut {
                sequence: "c"
                onActivated: {
                    targetsModel.compute()
                }
            }

            Shortcut {
                sequence: "r"
                onActivated: {
                    targetsModel.clearAllCells()
                    h3Model.clearAllCells()
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

            // Desktop: Shortcut hints panel
            Column {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 8
                spacing: 8
                z: 100
                visible: !window.isMobile

                ShortcutHint {
                    hintText: " Press 'a' to add a target "
                    textColor: "green"
                }

                ShortcutHint {
                    hintText: " Press 'c' to compute "
                    textColor: "yellow"
                }

                ShortcutHint {
                    hintText: " Press 'r' to clear all cells "
                    textColor: "red"
                }

                ShortcutHint {
                    hintText: " Press 'z' to reset zoom "
                    textColor: "white"
                }
            }

            // Mobile: Action buttons panel
            Column {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 16
                spacing: 12
                z: 100
                visible: window.isMobile

                // Menu button to show targets list
                RoundButton {
                    id: menuButton
                    width: 56
                    height: 56
                    text: "t"
                    font.pixelSize: 24
                    Material.background: Material.BlueGrey

                    onClicked: drawer.open()
                }

                // Add target button
                RoundButton {
                    width: 56
                    height: 56
                    text: "+"
                    font.pixelSize: 28
                    font.bold: true
                    Material.background: Material.Green

                    onClicked: {
                        targetsModel.requestCell(map.zoomLevel.toFixed(1), map.center)
                    }
                }

                // Compute button
                RoundButton {
                    width: 56
                    height: 56
                    text: "▶"
                    font.pixelSize: 24
                    Material.background: Material.Orange

                    onClicked: {
                        targetsModel.compute()
                    }
                }

                // Clear all button
                RoundButton {
                    width: 56
                    height: 56
                    text: "x"
                    font.pixelSize: 24
                    Material.background: Material.Red

                    onClicked: {
                        targetsModel.clearAllCells()
                        h3Model.clearAllCells()
                    }
                }

                // Reset zoom button
                RoundButton {
                    width: 56
                    height: 56
                    text: "z"
                    font.pixelSize: 24
                    Material.background: Material.Grey

                    onClicked: {
                        centerAnimation.to = map.center
                        zoomAnimation.to = 3
                        centerAnimation.start()
                        zoomAnimation.start()
                    }
                }
            }
            // Search statistics display (left on mobile to avoid button overlap)
            Rectangle {
                id: searchStats
                anchors.top: parent.top
                anchors.left: window.isMobile ? parent.left : undefined
                anchors.right: window.isMobile ? undefined : parent.right
                anchors.margins: 8
                border.color: "#66FFFFFF"
                border.width: 1
                color: "black"
                height: searchStatsTxt.implicitHeight + 10
                opacity: h3Model.searchStatsText ? 0.9 : 0
                radius: 7
                width: searchStatsTxt.implicitWidth + 20
                z: 100
                visible: h3Model.searchStatsText !== ""

                Behavior on opacity {
                    NumberAnimation { duration: 300 }
                }

                Text {
                    id: searchStatsTxt
                    anchors.centerIn: parent
                    font.pointSize: window.isMobile ? 12 : 14
                    font.family: "Helvetica"
                    color: "cyan"
                    text: h3Model.searchStatsText || ""
                }
            }
            // Отображение объединённых полигонов стен лабиринта
            Instantiator {
                id: mazePolygonsInstantiator
                model: h3Model.mazePolygons
                active: true

                delegate: MapPolygon {
                    id: mazePolyDelegate
                    path: modelData
                    opacity: 0.85
                    color: "pink"
                    border.color: "black"
                    border.width: 0.5
                    z: 1

                    Component.onCompleted: {
                        map.addMapItem(mazePolyDelegate)
                    }
                    Component.onDestruction: {
                        map.removeMapItem(mazePolyDelegate)
                    }
                }
            }

            // Круг границы допустимой области вокруг лабиринта
            MapCircle {
                id: mazeBoundaryCircle
                center: h3Model.mazeCenter
                radius: h3Model.mazeRadius
                color: "transparent"
                border.color: "red"
                border.width: 3
                opacity: 0.6
                z: 1
                referenceSurface: QtLocation.ReferenceSurface.Globe
                visible: h3Model.mazeCenter.isValid && h3Model.mazeRadius > 0
            }

            MapItemView {
                id: targetCells
                model: targetsModel ? targetsModel : null
                visible: true
                z: 20

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
                    id: cellPolygon

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
                z: 100

                Text {
                    id: copyRightTxt

                    color: "white"
                    text: " © MapTiler © OpenStreetMap contributors "
                }
            }
            // Zoom level indicator
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 8
                border.color: "#66FFFFFF"
                border.width: 1
                color: "darkslategray"
                height: zoomText.implicitHeight + 6
                opacity: 0.85
                radius: 7
                width: zoomText.implicitWidth + 20
                z: 100

                Text {
                    id: zoomText
                    anchors.centerIn: parent
                    color: "white"
                    text: "Zoom: %1".arg(map.zoomLevel.toFixed(1))
                }
            }
        }
    }

    // Компонент всплывающих уведомлений
    Popup {
        id: notificationPopup

        property string notificationType: "info"
        property string notificationMessage: ""

        anchors.centerIn: parent
        width: Math.min(parent.width * 0.8, 600)
        height: notificationText.implicitHeight + 40
        modal: false
        closePolicy: Popup.CloseOnPressOutside
        z: 1000

        background: Rectangle {
            color: {
                switch(notificationPopup.notificationType) {
                    case "warning": return "#FFA500"  // Orange
                    case "error": return "#FF4444"     // Red
                    case "critical": return "#8B0000"  // Dark Red
                    default: return "#4CAF50"          // Green
                }
            }
            opacity: 0.95
            radius: 8
            border.color: Qt.darker(color, 1.2)
            border.width: 2
        }

        contentItem: Text {
            id: notificationText
            text: notificationPopup.notificationMessage
            color: "white"
            font.pixelSize: 16
            font.bold: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            padding: 20
        }

        Timer {
            id: closeTimer
            interval: 3000
            running: false
            repeat: false
            onTriggered: notificationPopup.close()
        }

        onOpened: {
            closeTimer.start()
        }
    }

    // Обработчик сигналов уведомлений от targetsModel
    Connections {
        target: targetsModel

        function onShowNotification(message, type) {
            notificationPopup.notificationMessage = message
            notificationPopup.notificationType = type
            notificationPopup.open()
        }
    }

    // Mobile: Drawer with targets list
    Drawer {
        id: drawer
        width: window.width * 0.8
        height: window.height
        edge: Qt.LeftEdge
        visible: window.isMobile

        TargetsList {
            id: drawerTargetsList
            anchors.fill: parent

            onTargetClicked: function(coordinate, zoom) {
                centerAnimation.to = coordinate
                zoomAnimation.to = zoom
                centerAnimation.start()
                zoomAnimation.start()
                drawer.close()
            }
        }
    }
}
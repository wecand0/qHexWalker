#!/bin/bash
# =============================================================================
# qHexWalker AppImage Build Script
# =============================================================================
# Usage: ./packaging/appimage/build-appimage.sh [BUILD_DIR]
#
# Requirements:
#   - Built project (cmake --build)
#   - linuxdeploy with Qt plugin
#   - appimagetool (optional, linuxdeploy can create AppImage directly)
#
# Environment variables:
#   QT_DIR        - Path to Qt installation (e.g., /home/user/Qt/6.9.0/gcc_64)
#   MAPLIBRE_DIR  - Path to MapLibre installation
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${1:-${PROJECT_ROOT}/build/src}"
APPDIR="${BUILD_DIR}/AppDir"
APP_NAME="qHexWalker"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# =============================================================================
# Check prerequisites
# =============================================================================
check_prerequisites() {
    log_info "Checking prerequisites..."

    if [[ ! -f "${BUILD_DIR}/${APP_NAME}" ]]; then
        log_error "Executable not found: ${BUILD_DIR}/${APP_NAME}\nPlease build the project first: cmake --build ${BUILD_DIR}"
    fi

    if ! command -v linuxdeploy &> /dev/null; then
        log_warn "linuxdeploy not found. Downloading..."
        download_linuxdeploy
    fi

    if ! command -v linuxdeploy-plugin-qt &> /dev/null && [[ ! -f "${BUILD_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage" ]]; then
        log_warn "linuxdeploy-plugin-qt not found. Downloading..."
        download_linuxdeploy_qt_plugin
    fi
}

download_linuxdeploy() {
    local url="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    curl -L -o "${BUILD_DIR}/linuxdeploy-x86_64.AppImage" "$url"
    chmod +x "${BUILD_DIR}/linuxdeploy-x86_64.AppImage"
}

download_linuxdeploy_qt_plugin() {
    local url="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    curl -L -o "${BUILD_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage" "$url"
    chmod +x "${BUILD_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
}

# =============================================================================
# Prepare AppDir
# =============================================================================
prepare_appdir() {
    log_info "Preparing AppDir..."

    rm -rf "${APPDIR}"
    mkdir -p "${APPDIR}/usr/bin"
    mkdir -p "${APPDIR}/usr/lib"
    mkdir -p "${APPDIR}/usr/share/applications"
    mkdir -p "${APPDIR}/usr/share/icons/hicolor/256x256/apps"
    mkdir -p "${APPDIR}/usr/share/metainfo"

    # Copy executable
    cp "${BUILD_DIR}/${APP_NAME}" "${APPDIR}/usr/bin/"

    # Copy desktop file
    cp "${SCRIPT_DIR}/${APP_NAME}.desktop" "${APPDIR}/usr/share/applications/"

    # Copy or generate icon
    if [[ -f "${PROJECT_ROOT}/resources/icon.png" ]]; then
        cp "${PROJECT_ROOT}/resources/icon.png" "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png"
    else
        log_warn "Icon not found, generating placeholder..."
        generate_placeholder_icon
    fi

    # Copy data files (from source data/ folder, same as CMake does)
    if [[ -d "${PROJECT_ROOT}/data" ]]; then
        log_info "Copying data files..."
        cp -r "${PROJECT_ROOT}/data/"* "${APPDIR}/usr/bin/" 2>/dev/null || true
    fi
}

generate_placeholder_icon() {
    # Generate a simple SVG icon and convert to PNG
    cat > "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.svg" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <linearGradient id="bg" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" style="stop-color:#4CAF50"/>
      <stop offset="100%" style="stop-color:#2E7D32"/>
    </linearGradient>
  </defs>
  <rect width="256" height="256" rx="32" fill="url(#bg)"/>
  <g transform="translate(128,128)">
    <!-- Hexagon -->
    <polygon points="0,-80 69,-40 69,40 0,80 -69,40 -69,-40"
             fill="none" stroke="white" stroke-width="8"/>
    <!-- Path indicator -->
    <circle cx="-30" cy="-20" r="12" fill="white"/>
    <circle cx="30" cy="20" r="12" fill="#FF5722"/>
    <line x1="-20" y1="-12" x2="20" y2="12" stroke="white" stroke-width="4"/>
  </g>
</svg>
EOF
    # If imagemagick is available, convert to PNG
    if command -v convert &> /dev/null; then
        convert "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.svg" \
                "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png"
        rm "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.svg"
    else
        mv "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.svg" \
           "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png"
    fi
}

# =============================================================================
# Deploy with linuxdeploy
# =============================================================================
deploy_appimage() {
    log_info "Deploying AppImage..."

    local LINUXDEPLOY="${BUILD_DIR}/linuxdeploy-x86_64.AppImage"
    local QT_PLUGIN="${BUILD_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"

    # Set Qt environment
    if [[ -z "${QT_DIR}" ]]; then
        # Try to auto-detect Qt from qmake
        if command -v qmake &> /dev/null; then
            QT_DIR="$(qmake -query QT_INSTALL_PREFIX)"
            log_info "Auto-detected QT_DIR: ${QT_DIR}"
        else
            log_error "QT_DIR not set and qmake not found in PATH.\nPlease set QT_DIR environment variable (e.g., export QT_DIR=/home/user/Qt/6.9.0/gcc_64)"
        fi
    fi

    export QMAKE="${QT_DIR}/bin/qmake"
    export PATH="${QT_DIR}/bin:${PATH}"
    export LD_LIBRARY_PATH="${QT_DIR}/lib:${LD_LIBRARY_PATH:-}"
    export QML_SOURCES_PATHS="${PROJECT_ROOT}/ui"
    export QML_MODULES_PATHS="${QT_DIR}/qml"
    export EXTRA_QT_PLUGINS="positioning;sqldrivers"
    # Exclude problematic Qt geoservices plugins that cause ELF parsing errors
    export EXCLUDE_PLUGINS="geoservices"
    # Disable stripping which can cause issues with some libraries
    export NO_STRIP=1

    # Set MapLibre paths if provided
    if [[ -n "${MAPLIBRE_DIR}" ]]; then
        export LD_LIBRARY_PATH="${MAPLIBRE_DIR}/lib:${LD_LIBRARY_PATH:-}"
    fi

    # Run linuxdeploy (without creating AppImage yet - we need to add MapLibre plugin first)
    "${LINUXDEPLOY}" \
        --appdir "${APPDIR}" \
        --executable "${APPDIR}/usr/bin/${APP_NAME}" \
        --desktop-file "${APPDIR}/usr/share/applications/${APP_NAME}.desktop" \
        --icon-file "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png" \
        --plugin qt

    # Copy MapLibre geoservices plugin manually (since we excluded Qt's broken ones)
    copy_maplibre_plugin

    # Create final AppImage
    "${LINUXDEPLOY}" \
        --appdir "${APPDIR}" \
        --output appimage

    # Move AppImage to build directory
    mv ${APP_NAME}*.AppImage "${BUILD_DIR}/" 2>/dev/null || true

    log_info "AppImage created: ${BUILD_DIR}/${APP_NAME}*.AppImage"
}

# =============================================================================
# Copy MapLibre geoservices plugin
# =============================================================================
copy_maplibre_plugin() {
    log_info "Copying MapLibre geoservices plugin..."

    local GEOSERVICES_DIR="${APPDIR}/usr/plugins/geoservices"
    mkdir -p "${GEOSERVICES_DIR}"

    # Search for MapLibre plugin in common locations
    local MAPLIBRE_PLUGIN=""
    local SEARCH_PATHS=(
        "${MAPLIBRE_DIR}/lib/qt6/plugins/geoservices"
        "${MAPLIBRE_DIR}/plugins/geoservices"
        "${QT_DIR}/plugins/geoservices"
        "/usr/lib/x86_64-linux-gnu/qt6/plugins/geoservices"
        "/usr/lib/qt6/plugins/geoservices"
    )

    for path in "${SEARCH_PATHS[@]}"; do
        if [[ -f "${path}/libqtgeoservices_maplibregl.so" ]]; then
            MAPLIBRE_PLUGIN="${path}/libqtgeoservices_maplibregl.so"
            break
        elif [[ -f "${path}/libqtgeoservices_maplibre.so" ]]; then
            MAPLIBRE_PLUGIN="${path}/libqtgeoservices_maplibre.so"
            break
        fi
    done

    if [[ -n "${MAPLIBRE_PLUGIN}" ]]; then
        log_info "Found MapLibre plugin: ${MAPLIBRE_PLUGIN}"
        cp "${MAPLIBRE_PLUGIN}" "${GEOSERVICES_DIR}/"

        # Copy MapLibre libraries if not already present
        if [[ -n "${MAPLIBRE_DIR}" ]] && [[ -d "${MAPLIBRE_DIR}/lib" ]]; then
            log_info "Copying MapLibre libraries..."
            find "${MAPLIBRE_DIR}/lib" -name "*.so*" -type f -exec cp {} "${APPDIR}/usr/lib/" \; 2>/dev/null || true
        fi
    else
        log_warn "MapLibre geoservices plugin not found. Map functionality may not work."
        log_warn "Please set MAPLIBRE_DIR to your MapLibre installation directory."
    fi
}

# =============================================================================
# Main
# =============================================================================
main() {
    log_info "Building AppImage for ${APP_NAME}..."
    log_info "Project root: ${PROJECT_ROOT}"
    log_info "Build directory: ${BUILD_DIR}"

    check_prerequisites
    prepare_appdir
    deploy_appimage

    log_info "Done!"
}

main "$@"
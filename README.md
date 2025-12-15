# qHexWalker

Приложение на Qt 6, отображающее гексагональные ячейки H3 на карте через MapLibre Native Qt.

- H3 берётся из vcpkg (см. vcpkg.json)
- Для карт используется библиотека MapLibre Native Qt: https://github.com/maplibre/maplibre-native-qt

Скриншот:
https://github.com/user-attachments/assets/fddeafd0-e530-4c4f-9296-0b9d5bb9605f


## Требования

- CMake ≥ 3.19
- Компилятор с поддержкой C++20
- Qt 6.5+ (модули: Quick, QuickControls2, Concurrent, Network, Positioning)
- vcpkg (менеджер пакетов для C/C++)
- MapLibre Native Qt (собранный и установленный в систему или в локальный префикс)

Примечание: проект собирается через CMake и использует манифест vcpkg [vcpkg.json](vcpkg.json). Зависимости h3, spdlog, gtest, benchmark подтягиваются автоматически при указании toolchain-файла vcpkg.


## Быстрый старт (суммарно)

1) Установите Qt 6 (например, через официальный инсталлятор Qt или пакетный менеджер вашей ОС).
2) Установите vcpkg и подготовьте toolchain-файл.
3) Соберите и установите MapLibre Native Qt.
4) Соберите этот проект, указав CMAKE_TOOLCHAIN_FILE (vcpkg) и CMAKE_PREFIX_PATH (пути к Qt и MapLibre).

Детальные шаги ниже для Linux/macOS/Windows.


## 1. Установка Qt 6

Вам нужны компоненты: Qt Quick, Qt Quick Controls 2, Qt Concurrent, Qt Network, Qt Positioning.

- Linux (варианты):
  - Через официальный установщик Qt (рекомендуется для соответствия путям), либо
  - Через пакетный менеджер (названия пакетов зависят от дистрибутива: qt6-base-dev, qt6-declarative-dev, qt6-positioning-dev и т.п.).
  - Через aqt https://github.com/miurahr/aqtinstall
- macOS: через официальный установщик Qt или Homebrew: `brew install qt`.
- Windows: через официальный установщик Qt (MSVC или MinGW на ваш выбор). Убедитесь, что выбраны требуемые модули.

После установки запомните путь к Qt 6 (например):
- Linux: /opt/Qt/6.6.2/gcc_64
- macOS: /Users/you/Qt/6.6.2/macos
- Windows (MSVC): C:/Qt/6.6.2/msvc2022_64


## 2. Установка vcpkg

Пример установки (кроссплатформенно):

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh        # Linux/macOS
# или
./vcpkg/bootstrap-vcpkg.bat # Windows PowerShell/cmd
```

Toolchain-файл будет: <путь-к-vcpkg>/scripts/buildsystems/vcpkg.cmake

Манифест этого проекта (vcpkg.json) обеспечит установку пакетов: h3, spdlog, gtest, benchmark.


## 3. Сборка и установка MapLibre Native Qt

Проект использует компонент QMapLibre::Location, предоставляемый библиотекой MapLibre Native Qt. Её необходимо собрать и установить заранее.

Сборка (универсальный пример):

```bash
git clone https://github.com/maplibre/maplibre-native-qt.git
cd maplibre-native-qt

# Папка установки (можете выбрать другую):
export MAPLIBRE_INSTALL_PREFIX="$HOME/.local/maplibre-native-qt"

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$MAPLIBRE_INSTALL_PREFIX" \
  -DCMAKE_PREFIX_PATH="/путь/к/Qt/6.x.x/<platform>" \
  -DCMAKE_TOOLCHAIN_FILE="/путь/к/vcpkg/scripts/buildsystems/vcpkg.cmake"

cmake --build build --config Release -j
cmake --install build --config Release
```

Где `-DCMAKE_PREFIX_PATH` указывает на установленный Qt 6 (см. раздел 1).
После установки запомните `MAPLIBRE_INSTALL_PREFIX` — его нужно будет добавить в `CMAKE_PREFIX_PATH` при сборке qHexWalker.

Документация MapLibre Native Qt: https://maplibre.org/maplibre-native-qt/docs/


## 4. Сборка qHexWalker

Минимально нужно указать:
- `CMAKE_TOOLCHAIN_FILE` — путь к vcpkg toolchain
- `CMAKE_PREFIX_PATH` — список путей, где CMake будет искать Qt и MapLibre Native Qt

Также доступны опции CMake:
- `-DBUILD_TESTS=ON|OFF` (по умолчанию ON)
- `-DBENCHMARK_ENABLE=ON|OFF` (по умолчанию ON)
- `-DDEBUG=ON|OFF` (влияет на тип сборки и флаги)

### Linux/macOS

```bash
git clone https://github.com/your-org-or-user/qHexWalker.git
cd qHexWalker

export VCPKG_ROOT="/путь/к/vcpkg"
export QT_PREFIX="/путь/к/Qt/6.x.x/<platform>"       # см. раздел 1
export MAPLIBRE_PREFIX="$HOME/.local/maplibre-native-qt" # см. раздел 3

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_PREFIX_PATH="${QT_PREFIX};${MAPLIBRE_PREFIX}"

cmake --build build -j

# Запуск тестов (если включены)
ctest --test-dir build --output-on-failure

# Установка (необязательно)
cmake --install build
```

Типичные пути QT_PREFIX:
- Linux: `/opt/Qt/6.6.2/gcc_64`
- macOS: `/Users/you/Qt/6.6.2/macos`


### Windows (PowerShell)

```powershell
git clone https://github.com/your-org-or-user/qHexWalker.git
cd qHexWalker

$env:VCPKG_ROOT = "C:/src/vcpkg"
$env:QT_PREFIX   = "C:/Qt/6.6.2/msvc2022_64"
$env:MAPLIBRE_PREFIX = "$env:USERPROFILE/.local/maplibre-native-qt"

cmake -S . -B build `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_PREFIX_PATH="$env:QT_PREFIX;$env:MAPLIBRE_PREFIX"

cmake --build build --config Release -j

# Тесты
ctest --test-dir build -C Release --output-on-failure

# Установка (опционально)
cmake --install build --config Release
```


## Запуск

После сборки исполняемый файл находится в каталоге сборки:
- Linux/macOS: `build/QHexWalker` (или `qHexWalker` в зависимости от настроек)
- Windows: `build/Release/QHexWalker.exe`

Приложение использует QML и плагин MapLibre Location. В CMake проекте вызывается `qmaplibre_location_setup_plugins(...)`, что упаковывает необходимые плагины при установке/деплое. Для запуска из каталога сборки убедитесь, что Qt-плагины доступны (обычно проблем нет при запуске рядом с собранными артефактами или после `cmake --install`).


## Частые проблемы

- «Could not find a package configuration file provided by "QMapLibre" …»
  - Проверьте, что MapLibre Native Qt установлен, и его префикс добавлен в `CMAKE_PREFIX_PATH`.
- «Could not find Qt6::Positioning»
  - Установите модуль Qt Positioning и добавьте корректный путь Qt в `CMAKE_PREFIX_PATH`.
- «vcpkg toolchain not found»
  - Проверьте путь к `vcpkg.cmake` и наличие `bootstrap-vcpkg`.


## Разработка в IDE

- Qt Creator/CLion/VS Code: укажите toolchain-файл vcpkg и `CMAKE_PREFIX_PATH` с путями к Qt и MapLibre. На macOS и Windows проще всего выбирать Qt из установленного набора SDK.

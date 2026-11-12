[**English**](README.md) · [Русский](README.ru.md)

# YamlEditor

A desktop GUI editor for YAML configuration files, built with Qt 6 and C++17.
Edit keys and values in a tree view, filter by key, search and replace across the
document, and work with several files at once in tabs — with light and dark themes
and English / Russian localization.

---

## Features

- **Tree-based editing** — YAML is parsed into an expandable key/value tree; edit keys and values inline.
- **Type highlighting** — strings, numbers, booleans and nulls are coloured differently; maps and lists show their size (`{3}`, `[2]`).
- **Safe round-trip** — lists of objects, nested lists, `null`, empty collections and quoted strings are written back unchanged.
- **Key filter** — hide any keys in the side panel to focus on the fields you need.
- **Inline find & replace** — `Ctrl+F` / `Ctrl+H`, with a match counter, case-sensitive and regex options; *Replace all* is a single undo step.
- **Multi-file tabs** — each file keeps its own undo history, expanded nodes and filter; unsaved files are marked with ●.
- **Files panel and drag & drop** — every YAML file of the working folder is listed; drop files or folders onto the window to open them.
- **Light / dark / system theme** — one palette for the whole app, switchable at runtime.
- **Localization** — switch the interface language at runtime (English / Russian); the choice is remembered.
- **Optional Yandex.Disk sync** — upload saved files to Yandex.Disk (see [Optional: Yandex.Disk](#optional-yandexdisk)).

## Built with

- **Qt 6.8+** — Widgets, Network, Concurrent, Svg
- **C++17**
- **CMake** (3.5+)
- **[yaml-cpp](https://github.com/jbeder/yaml-cpp)** — fetched automatically via CMake `FetchContent`

## Requirements

- A C++17 compiler (MinGW, MSVC, GCC, or Clang)
- Qt 6.8 or newer with the Widgets, Network, Concurrent and Svg modules
- CMake 3.5 or newer
- Internet access on the first build (CMake downloads yaml-cpp)

## Building

### With Qt Creator

1. Open `CMakeLists.txt` as a project.
2. Select a Qt 6.8+ kit.
3. Build and run. On the first configure, CMake fetches yaml-cpp automatically.

### From the command line

```bash
git clone https://github.com/Ruha24/YamlEditor.git
cd YamlEditor

cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<compiler>
cmake --build build

# run
./build/editor          # Linux
.\build\editor.exe      # Windows
```

Replace `CMAKE_PREFIX_PATH` with the path to your Qt installation.

## Usage

| Action | Shortcut |
|---|---|
| Open file / folder | `Ctrl+O` / `Ctrl+Shift+O` |
| Save / save all | `Ctrl+S` / `Ctrl+Shift+S` |
| Close tab | `Ctrl+W` or middle click |
| Undo / redo | `Ctrl+Z` / `Ctrl+Y` |
| Find / replace | `Ctrl+F` / `Ctrl+H` |
| Edit a cell | double click or `F2` |
| Add child / delete node | `Insert` / `Delete` (or right click) |
| Show log | `` Ctrl+` `` |
| Full screen | `F11` |

Theme and language are in the **View** menu.

## Localization

The interface ships with English (`en_GB`) and Russian (`ru_RU`) translations.
Source strings are wrapped in `tr(...)`; translation files live in the project's
`.ts` files and are compiled to `.qm` at build time.

To update or add translations:

1. Run the `update_translations` CMake target to collect strings into the `.ts` files.
2. Edit the `.ts` files in **Qt Linguist**.
3. Rebuild — the `.qm` files are regenerated and embedded as resources.

## Optional: Yandex.Disk

The app can upload edited files to Yandex.Disk. This is optional and the editor
works fully without it.

Open **Account → Connect Yandex Disk...** and paste an OAuth token. On Windows the
token is encrypted with DPAPI; it is never stored in the source. When connected,
saved files are uploaded to `/YamlFiles` on the disk and newer cloud copies are
downloaded on start (a local file that is newer than the cloud copy is never overwritten).

## Project structure

```
src/
├── main.cpp
├── mainwindow.*               # main window: menus, toolbar, tabs, tree, docks
├── theme/theme.*              # colour tokens, palette, stylesheet, tinted icons
├── customWidget/
│   ├── findbar.*              # inline find / replace strip
│   ├── filelistpanel.*        # "Files" side panel
│   ├── keyfilterpanel.*       # "Key filter" side panel
│   ├── logpanel.*             # log dock
│   └── connectdialog.*        # Yandex.Disk token dialog
├── controller/                # search, open documents, file watching
├── files/
│   ├── filesystem.*           # known files
│   ├── filedownloadtask.*     # background download task
│   ├── snapshotcommand.h      # undo step
│   └── yaml/
│       ├── yamlnode.*         # in-memory YAML tree
│       ├── yamlreader.*       # parse / emit via yaml-cpp
│       ├── yamltreemodel.*    # Qt model for the tree view
│       ├── yamlitemdelegate.* # cell editor and type colouring
│       └── yamlvalidator.*    # key validation
├── security/tokenstore.*      # encrypted token storage
├── api/yandex/                # optional Yandex.Disk integration
├── assets/icons/              # SVG icons (tinted at runtime)
└── translations/              # .ts files
```

## License

See the repository for license details.

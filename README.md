[**English**](README.md) · [Русский](README.ru.md)

# YamlEditor

A desktop GUI editor for YAML configuration files, built with Qt 6 and C++17.
Edit keys and values in a tree view, filter by key, search and replace across the
document, and work with several files at once in tabs — all in a dark, themed UI
with English / Russian localization.

---

## Features

- **Tree-based editing** — YAML is parsed into an expandable key/value tree; edit values inline.
- **Key filter chips** — toggle which keys are shown via flow-laid checkbox chips.
- **Search & replace** — find text in the tree, with case-sensitive and regex options, plus replace / replace-all.
- **Multi-file tabs** — open one file, several files, or a whole folder of `*.yml` / `*.yaml`; each file gets its own tab.
- **Drag & drop** — drop YAML files onto the window to open them.
- **Persisted view state** — expanded nodes stay expanded across redraws.
- **Dark theme** — a consistent dark / teal style across all windows and dialogs.
- **Localization** — switch the interface language at runtime (English / Russian).
- **Optional Yandex.Disk sync** — upload edited files to Yandex.Disk (see [Optional: Yandex.Disk](#optional-yandexdisk)).

## Built with

- **Qt 6** (Qt 5 also supported) — Widgets, Network, Concurrent
- **C++17**
- **CMake** (3.5+)
- **[yaml-cpp](https://github.com/jbeder/yaml-cpp)** — fetched automatically via CMake `FetchContent`

## Requirements

- A C++17 compiler (MinGW, MSVC, GCC, or Clang)
- Qt 6 (or Qt 5) with the Widgets, Network, and Concurrent modules
- CMake 3.5 or newer
- Internet access on the first build (CMake downloads yaml-cpp)

## Building

### With Qt Creator

1. Open `CMakeLists.txt` as a project.
2. Select a kit (Qt 6 recommended).
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

- **Open folder** — pick a folder; all `*.yml` / `*.yaml` files in it open as tabs.
- **Open file** — pick one or more YAML files.
- **Drag & drop** — drop files onto the window.
- **Edit** — expand the tree and edit values inline; right-click a key/value for add / delete.
- **Display** — redraw the current tree from the in-memory document.
- **Save** — write the active file back to disk.
- **Value keys** — toggle chips to control which keys appear in the tree.
- **Language** — use the language selector in the top bar to switch interface language.

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

The access token is **not** stored in the source. Provide your own at runtime
(for example via a local `config.ini`, which is git-ignored). A
`config.example.ini` template is included. Never commit a real token.

## Project structure

```
src/
├── main.cpp
├── mainwindow.*              # main window: tabs, tree, key filter
├── editwidget.*             # add/delete row dialog
├── searchingwindow.*        # search dialog
├── replacewindow.*          # replace dialog
├── customWidget/
│   ├── customlineedit.*     # inline-editable tree cell
│   └── flowlayout.*         # wrapping layout for key chips
├── files/
│   ├── filesystem.*         # local file handling
│   ├── filedownloadtask.*   # background download task
│   └── yaml/
│       ├── yamlnode.*       # in-memory YAML tree model
│       └── yamlreader.*     # parse / emit via yaml-cpp
├── api/yandex/              # optional Yandex.Disk integration
└── assets/                  # icons and resources
```

## License

See the repository for license details.

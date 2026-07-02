# Continuous Integration

The `ci.yml` workflow runs on every push and pull request to `main`.

## Jobs

| Job | Runner | What it does |
|-----|--------|--------------|
| **Build & test** | Ubuntu + Windows | Configures with `-DBUILD_TESTING=ON`, builds Release, runs the full CTest suite |
| **clang-format** | Ubuntu | Verifies every file under `src/` and `tests/` matches `.clang-format` |
| **clang-tidy**   | Ubuntu | Builds with `compile_commands.json`, then lints `src/*.cpp` against `.clang-tidy` |

## Before enabling CI: format the code once

The `clang-format` job is strict (`--Werror`), so any pre-existing
formatting drift will fail the very first run. Format the whole tree once
and commit the result before (or together with) enabling CI:

```bash
# Linux / macOS
find src tests -type f \( -name '*.cpp' -o -name '*.h' \) \
  | xargs clang-format -i --style=file

# Windows (PowerShell)
Get-ChildItem -Recurse src,tests -Include *.cpp,*.h |
  ForEach-Object { clang-format -i --style=file $_.FullName }
```

Review the diff (`git diff`) — the `.clang-format` here targets the Qt
style the project already uses, so changes should be minimal (mostly
line-wrapping and include alignment).

## Running the checks locally

```bash
# format check (no changes, just report)
clang-format --dry-run --Werror --style=file $(find src tests -name '*.cpp' -o -name '*.h')

# clang-tidy (needs a configured build with compile_commands.json)
cmake -B build -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
clang-tidy -p build $(find src -name '*.cpp' ! -name 'main.cpp')
```

## Notes

- **Qt version** is pinned to `6.7.*` via `install-qt-action`. Bump it in
  `ci.yml` when you move to a newer Qt.
- **yaml-cpp** is fetched by CMake (`FetchContent`) during configure, so no
  extra install step is needed. Consider pinning `GIT_TAG` to a release
  tag instead of `master` for reproducible CI builds.
- The clang-tidy job **builds first** so that AUTOMOC/AUTOUIC generate the
  `moc_*` and `ui_*.h` headers the sources include.

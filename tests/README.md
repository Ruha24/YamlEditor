# Unit tests

Headless Qt Test suite for the GUI-free core of YamlEditor.

## Coverage

| Test | Under test | Focus |
|------|-----------|-------|
| `tst_yamlnode`   | `YamlNode`   | tree mutation (`AddKeyWithValue`, `AddValueToKey`, `RemoveKey`, `RemoveValue`), `FindNodeByKey` const/non-const |
| `tst_yamlreader` | `YamlReader` | parse flat/nested maps, error paths (missing/invalid file), read→save→read round-trip |
| `tst_filesystem` | `FileSystem` | directory scan, extension filtering, path lookup, checksum stability |

## Running

```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Or run a single test binary directly for verbose output:

```bash
./build/tests/tst_yamlnode -v2
```

# Setup — activating ESP-IDF

Every `idf.py` command (`build`, `flash`, `monitor`, `set-target`, …) needs the
ESP-IDF environment active in your shell: the toolchain on `PATH` and the Python
venv activated.

## Activate the environment

**On this machine, always use the custom activation script:**

```sh
source ~/.espressif/tools/activate_idf_v5.5.3.sh
```

This is already sourced from `~/.zshrc`, so a normal interactive terminal is
ready to go. You only need to run it manually in a fresh non-login shell, a
script, or a subshell.

## ⚠️ Do NOT use the stock `export.sh`

The standard `source $IDF_PATH/export.sh` is **broken on this setup**. It fails
with:

```
KeyError: 'IDF_PATH_OLD'
ERROR: Activation script failed
```

When that happens the compiler never lands on `PATH`, and the next build dies
with `xtensa-esp32-elf-gcc ... is not a full path and was not found in the PATH`.
The custom `activate_idf_v5.5.3.sh` avoids the bug and sets everything correctly.
See [troubleshooting.md](troubleshooting.md) for the full symptom → fix.

## Verify it worked

```sh
which xtensa-esp32-elf-gcc
# -> /Users/eduardo/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20251107/xtensa-esp-elf/bin/xtensa-esp32-elf-gcc

idf.py --version
# -> ESP-IDF v5.5.3
```

If `which` prints nothing, the environment is not active — re-source the script.

## Environment facts

| Thing | Value |
|-------|-------|
| ESP-IDF version | v5.5.3 |
| `IDF_PATH` | `~/.espressif/v5.5.3/esp-idf` |
| Tools path | `~/.espressif/tools` |
| Toolchain | unified `xtensa-esp-elf` (esp-14.2.0), provides `xtensa-esp32-elf-gcc` |
| Python venv | `~/.espressif/tools/python/v5.5.3/venv` |

Note: there is **no** old-style `tools/xtensa-esp32-elf/` directory. The chip
compilers come from the single unified `xtensa-esp-elf` toolchain.

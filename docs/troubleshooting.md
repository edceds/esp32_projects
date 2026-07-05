# Troubleshooting

Problems actually seen on this setup, and their fixes.

## `xtensa-esp32-elf-gcc ... not found` during build or set-target

```
CMake Error: The CMAKE_CXX_COMPILER: xtensa-esp32-elf-g++
  is not a full path and was not found in the PATH.
```

**Cause:** the ESP-IDF environment isn't active in this shell — the compiler
isn't on `PATH`.

**Fix:** activate it, then retry:

```sh
source ~/.espressif/tools/activate_idf_v5.5.3.sh
which xtensa-esp32-elf-gcc     # should print a path now
```

Do **not** use `source $IDF_PATH/export.sh` — see the next entry.

## `KeyError: 'IDF_PATH_OLD'` from export.sh

```
ERROR: Activation script failed
...
File ".../export_utils/utils.py", line 20, in __init__
    self.IDF_PATH_OLD = os.environ['IDF_PATH_OLD']
KeyError: 'IDF_PATH_OLD'
```

**Cause:** a bug in the stock `export.sh` / `activate_venv.py` flow in this
ESP-IDF v5.5.3 install. It expects `IDF_PATH_OLD` to already be set and aborts
when it isn't, so the toolchain never gets added to `PATH`.

**Fix:** never source `export.sh`. Use the custom script instead:

```sh
source ~/.espressif/tools/activate_idf_v5.5.3.sh
```

## `idf.py` runs but build fails only in scripts / subshells

`idf.py` itself is aliased straight to the venv Python, so commands that don't
compile (like `create-project`) work even without activation. But anything that
invokes the compiler (`build`, `set-target`) needs the full environment.

**Fix:** source the activation script inside the script/subshell before calling
`idf.py build`.

## Serial port not found / can't flash

**Fix:** confirm the board enumerated and pass the port explicitly:

```sh
ls /dev/cu.usbserial-*
idf.py -p /dev/cu.usbserial-5B153806091 flash monitor
```

If nothing shows up, replug the USB cable (some cables are power-only) and check
for a CP210x / CH340 USB-UART driver. See [hardware.md](hardware.md).

## Changed target or config and things act weird

`set-target` and some `menuconfig` changes invalidate the build. When in doubt:

```sh
idf.py fullclean
idf.py build
```

## DHT22-specific: `no response` or `checksum mismatch`

Not a toolchain issue — it's wiring/timing. Check the 10 kΩ pull-up on the data
line and that the sensor had ~1–2 s to warm up after power. See the
`temp_humid_sensor` project README.

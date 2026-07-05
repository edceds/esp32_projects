# Workflow — new project, build, flash, monitor

Assumes the environment is active — see [setup.md](setup.md).

## Create a new project

Don't copy an existing project. ESP-IDF scaffolds the minimal skeleton for you:

```sh
cd ~/esp32_projects
idf.py create-project --path my_project my_project
cd my_project
idf.py set-target esp32     # classic ESP32; regenerates sdkconfig
```

That's it — write your code in `main/my_project.c`.

### What gets generated

```
my_project/
├── CMakeLists.txt          # project-level: project name
└── main/
    ├── CMakeLists.txt       # registers source files (add new .c files here)
    └── my_project.c         # your code, with an empty app_main()
```

`set-target` additionally generates `sdkconfig` and a `build/` directory. Both
are machine-generated; keep `build/` out of git.

### Start from a working example instead

```sh
idf.py create-project-from-example "example/path"
```

Browse `$IDF_PATH/examples/` or run
`idf.py create-project-from-example --help` to see what's available (Wi-Fi,
HTTP, I2C, SPI, BLE, …).

## Build · flash · monitor

```sh
idf.py build            # compile
idf.py flash            # write to the board (auto-detects the port if one board)
idf.py monitor          # serial console; exit with Ctrl-]
idf.py flash monitor    # flash then immediately open the monitor
```

If port auto-detection fails, name it explicitly:

```sh
idf.py -p /dev/cu.usbserial-5B153806091 flash monitor
```

(Find the port with `ls /dev/cu.usbserial-*`. See [hardware.md](hardware.md).)

## Common `idf.py` commands

| Command | Does |
|---------|------|
| `idf.py build` | Compile the project |
| `idf.py flash` | Flash the board |
| `idf.py monitor` | Open serial monitor (`Ctrl-]` to quit) |
| `idf.py flash monitor` | Flash, then monitor |
| `idf.py -p PORT ...` | Target a specific serial port |
| `idf.py set-target esp32` | Choose the chip (⚠ regenerates `sdkconfig`) |
| `idf.py menuconfig` | Interactive project configuration |
| `idf.py size` | Show binary size breakdown |
| `idf.py fullclean` | Delete the `build/` directory |
| `idf.py app-flash` | Flash only the app (skip bootloader/table) — faster iterations |

## Project layout convention

Keep new projects **flat and in plain C**:

```
my_project/
├── CMakeLists.txt
├── README.md
├── .gitignore          # at least: build/
├── main/
│   ├── CMakeLists.txt
│   └── my_project.c
└── docs/               # optional, project-specific notes
```

The older `esp_http_test` project wraps everything in a `firmware/` subfolder
and uses C++ with `extern "C"` around the IDF headers. That was hand-rolled and
is **not** required — don't replicate it unless a project genuinely needs C++.

## Adding source files

New `.c`/`.cpp` files won't build until you list them in `main/CMakeLists.txt`:

```cmake
idf_component_register(SRCS "my_project.c" "sensor.c" "wifi.c"
                    INCLUDE_DIRS ".")
```

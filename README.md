# esp32_projects

ESP32 projects and general working notes for ESP-IDF on this machine.

## Projects

| Project | What it does |
|---------|--------------|
| [temp_humid_sensor](temp_humid_sensor/) | Read temperature + humidity from a DHT22 sensor |
| [esp_http_test](esp_http_test/) | HTTP client posting GPIO/chip status to a server |

## General docs

Start here — these apply to every project:

- **[docs/setup.md](docs/setup.md)** — activating the ESP-IDF environment (and the `export.sh` gotcha on this machine)
- **[docs/workflow.md](docs/workflow.md)** — creating a new project, build / flash / monitor, common `idf.py` commands
- **[docs/troubleshooting.md](docs/troubleshooting.md)** — errors seen on this setup and their fixes
- **[docs/hardware.md](docs/hardware.md)** — the board on hand, serial port, wiring notes

## Quickstart

```sh
source ~/.espressif/tools/activate_idf_v5.5.3.sh   # once per shell
cd ~/esp32_projects
idf.py create-project --path my_project my_project
cd my_project
idf.py set-target esp32
idf.py build flash monitor
```

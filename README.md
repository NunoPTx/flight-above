# flight-above

Shows the closest aircraft flying over your location on a HUB75 64x32 LED matrix, driven by an ESP32.

## Content

- `esp32.cpp`: main firmware
- `area.png`: example search area
- `hub75-pinout`: pinout for the HUB75 model used
- `esp32-pinout`: pinout for the ESP32 model used

## What it shows

- Airline name (from callsign)
- Aircraft model
- Origin or destination airport
- Climb/descent arrow
- Speed (kts) and altitude (ft)
- Live data from the Flightradar24 feed, refreshed every 10 s

## Setup

Before flashing, edit these values at the top of `V-2.cpp`:

| Variable | Description |
|---|---|
| `WIFI_SSID` | Wi-Fi network name (**2.4 GHz only**) |
| `WIFI_PASS` | Wi-Fi password |
| `HOME_LAT` / `HOME_LON` | Your location (red dot), the center used to pick the nearest aircraft |
| `LAMIN` / `LAMAX` | South/North edges of the area |
| `LOMIN` / `LOMAX` | West/East edges of the area |

## Search area

![Search area](area.png)

- **Green dots:** the corners of the search box (`LAMIN`, `LAMAX`, `LOMIN`, `LOMAX`). Only aircraft inside this box are considered.
- **Red dot:** your home / desired tracking center (`HOME_LAT`, `HOME_LON`). The aircraft closest to this point is shown.

## Wiring

| HUB75 pin | ESP32 GPIO |
|---|---|
| R1 | 25 |
| G1 | 26 |
| B1 | 27 |
| R2 | 14 |
| G2 | 32 |
| B2 | 13 |
| A | 23 |
| B | 19 |
| C | 5 |
| D | 17 |
| LAT | 4 |
| OE | 15 |
| CLK | 16 |
| GND | GND |

## Libraries

- ESP32-HUB75-MatrixPanel-I2S-DMA
- ArduinoJson

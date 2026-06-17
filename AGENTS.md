# BitBadge — Agent Instructions

## Project
Dump firmware from a Nuvoton MS51 (1T 8051, typically 16KB flash) via **ICP** protocol using an ESP32-C6 as a bit-banged programmer.

## Build & Flash
```bash
pio run --target upload
```
Single build target, no test/lint/typecheck infrastructure.

## Status
**ICP entry is failing.** Device ID reads 0xFFFF (with internal pull-ups) or 0x0000 (without) — the MS51 is not driving DAT. The software is believed correct (verified against [NuMicro-8051-prog](https://github.com/nikitalita/NuMicro-8051-prog) reference). Hardware issue suspected — likely missing external 100kΩ pull-ups on ICE_DAT and ICE_CLK that the datasheet recommends.

## Architecture (layered)
- `src/n51_pgm.h` — platform abstraction (GPIO, timing, print)
- `src/n51_pgm_esp32.c` — ESP32-C6 GPIO implementation on **GPIO4=CLK, GPIO5=DAT, GPIO6=RST**
- `src/n51_icp.h` / `src/n51_icp.c` — ICP protocol core (read commands only; all write/erase stripped)
- `src/main.c` — entrypoint: init → enter ICP → read device/UID/flash → exit

## Critical constraints
- **READ-ONLY ONLY.** ICP protocol layer contains *no* write, erase, or program commands. The target cannot be modified by this firmware.
- Entry sequence tries 3 attempts: standard reset (0x9E1CB6) + entry (0x5AA503), then alternate (0xAE1CB6 + 0x0075BA03).
- **Critical timing**: only **100µs** between reset-sequence end and entry-bits start (any longer misses the ICP entry window).
- Serial output at **115200 baud**. Capture with `picocom -b 115200 /dev/ttyACM0 --log capture.log`.
- Board is 3.3V. Datasheet recommends 100kΩ pull-ups on ICE_DAT/ICE_CLK, 10kΩ + 10µF on nRESET.
- MS51EB9AE marking is **not** in the official Nuvoton part list; closest are MS51FB9AE (16KB) and MS51EB0AE (32KB).

## Pinout (verified by user)
| Pad | Signal    | GPIO |
|-----|-----------|------|
| D   | ICE_DAT   | 5    |
| C   | ICE_CLK   | 4    |
| R   | nRESET    | 6    |
| V   | 3.3V      | —    |
| G   | GND       | —    |

## What needs work
- ICP entry is **blocked** (devid=0xFFFF/0x0000). Likely cause: external 100kΩ pull-ups on DAT and CLK needed.
- No tests exist (`test/` has only a PlatformIO placeholder).

## If retrying ICP entry
The code in `src/n51_icp.c` matches the proven reference. Changes to timing or entry sequences are unlikely to help. Focus on hardware:
- Add external 100kΩ pull-ups from DAT and CLK to 3.3V
- Verify power on V pad with multimeter
- Try a hard power-cycle (toggle VCC via a spare ESP32 GPIO connected to the V pad)
- Verify DAT actually toggles during entry with a logic analyzer or oscilloscope
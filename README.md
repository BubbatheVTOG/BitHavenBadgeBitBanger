# BitBadge - Nuvoton MS51 Firmware Dumper

Attempts to dump firmware from a Nuvoton MS51 (1T 8051 MCU) via **ICP** (In-Circuit Programming) using an ESP32-C6 as a bit-banged programmer.

## Board

![Board top](https://photos.fife.usercontent.google.com/pw/AP1GczNKi3WFrrLdX-q_2sbdkwE9FwFy1D3cqQ8TFaVRiSGYF9L8HCabtXBCuw=w1101-h1463-s-no-gm?authuser=0)
![Board bottom](https://photos.fife.usercontent.google.com/pw/AP1GczPF-ziFjVcIkD5HZuDfgKpZfBZuGbzex_Tz0Kn4KQJLROJXZtM_8_RVzg=w1101-h1463-s-no-gm?authuser=0)
![Board detail](https://photos.fife.usercontent.google.com/pw/AP1GczN-HoLHQUnUPS4OmefHKUGm-dTj7LJCTg8cMhBlJx6zGA5yHiRrpJcy8w=w1943-h1463-s-no-gm?authuser=0)


## ⚠️ Current Status: ICP Entry Not Working

Despite the implementation matching the proven [NuMicro-8051-prog](https://github.com/nikitalita/NuMicro-8051-prog) reference, ICP entry fails: device ID reads **0xFFFF** (chip not driving DAT) or **0x0000** (DAT floating low without pull-ups). The software is believed correct; the issue is hardware.

## Target IC

| Part       | Description             |
|------------|-------------------------|
| MS51 family | 1T 8051, 16KB Flash (typical), 1KB SRAM |
| Core       | 8051 @ up to 24 MHz     |

Note: "MS51EB9AE" marking is **not** in the official Nuvoton part list. Closest matches: MS51FB9AE (16KB, devid `0x0B004B21`) or MS51EB0AE (32KB, devid `0x02005322`). The ICP entry sequence is family-wide, so it should work either way.

## Mystery Board Pinout (Verified by User)

The board has 5 pads:

| Pad | Signal    | Function        |
|-----|-----------|-----------------|
| D   | ICE_DAT   | ICP Data (bi-dir) |
| C   | ICE_CLK   | ICP Clock       |
| R   | nRESET    | Reset           |
| V   | VCC       | 3.3V Power      |
| G   | GND       | Ground          |

## Wiring to ESP32-C6 (Seeed Xiao ESP32C6)

| MS51 Pad | ESP32-C6 GPIO | Xiao Pin |
|----------|---------------|----------|
| V        | 3.3V          | 3V3      |
| G        | GND           | GND      |
| R        | GPIO6         | D6       |
| D        | GPIO5         | D5       |
| C        | GPIO4         | D4       |

### Recommended External Components

The datasheet recommends:
- **100kΩ pull-ups** on ICE_DAT (D) and ICE_CLK (C) to VCC
- **10kΩ pull-up** + **10µF capacitor** on nRESET (R) — long leg of cap to R pad, short leg to GND

The ESP32-C6 internal pull-ups (~45kΩ) are enabled in firmware as a substitute, but they may not be sufficient. With internal pull-ups only, the result is `devid=0xFFFF`; without them, `devid=0x0000`. Both indicate the chip is not driving DAT.

**Board is 3.3V** — do NOT connect V to 5V.

## Build & Flash the Dumper

```bash
pio run --target upload
```

## Usage

1. Wire the MS51 board to the ESP32-C6
2. Power the ESP32-C6 via USB
3. Open a serial monitor at 115200 baud:

```bash
picocom -b 115200 /dev/ttyACM0
```

4. The dumper will:
   - Enter ICP mode on the MS51
   - Print chip identification (Device ID, Product ID, UID, UCID)
   - Dump the full 16KB flash as a hex dump
   - Exit ICP mode

5. Capture the output to a file and extract the hex dump:

```bash
picocom -b 115200 /dev/ttyACM0 --log capture.log
```

Then convert to binary:

```bash
grep -E '^[0-9A-F]{4}  ' capture.log | sed 's/^....  //' | sed 's/ //g' > dump.hex
xxd -r -p dump.hex firmware.bin
```

## Protocol

Uses the Nuvoton **ICP** (In-Circuit Programming) protocol over 3 wires:
- **ICE_DAT** — bidirectional serial data
- **ICE_CLK** — clock from programmer
- **nRESET** — reset to enter/exit ICP mode

### Entry Sequence (3 attempts)

The firmware tries up to 3 times with different sequences:

| Attempt | Reset Seq  | Entry Code   |
|---------|------------|--------------|
| 1       | `0x9E1CB6` | `0x5AA503`   |
| 2       | `0x9E1CB6` | `0x5AA503`   |
| 3       | `0xAE1CB6` | `0x0075BA03` |

Reset bit timing: 10ms/bit (per reference). Entry bit timing: 60µs/bit.
Critical: only **100µs delay** between reset sequence end and entry bits start.

### ICP Commands (read-only)

| Command       | Opcode | Description |
|---------------|--------|-------------|
| READ_DEVICE_ID | `0x0c` | Returns 16-bit device ID |
| READ_UID       | `0x04` | 12-byte unique ID |
| READ_CID       | `0x0b` | 1-byte customer ID |
| UCID (extended) | `0x04` (addr 0x20+) | 16-byte UCID |
| READ_FLASH     | `0x00` | Read flash by address |

### Exit Sequence

`RST high → 5ms → RST low → 10ms → send exit bits (0xF78F0) → 500µs → RST high`

## Architecture (layered)

- `src/n51_pgm.h` — platform abstraction (GPIO, timing, print)
- `src/n51_pgm_esp32.c` — ESP32-C6 GPIO implementation on **GPIO4=CLK, GPIO5=DAT, GPIO6=RST**
- `src/n51_icp.h` / `src/n51_icp.c` — ICP protocol core (read commands only; all write/erase stripped)
- `src/main.c` — entrypoint: init → enter ICP → read device/UID/flash → exit

The implementation is a port of [NuMicro-8051-prog](https://github.com/nikitalita/NuMicro-8051-prog) (MIT license) with all write/erase functions removed.

## Troubleshooting

If you see `devid=0xFFFF`:
- Verify wiring (continuity from each pad to its GPIO)
- Add external 100kΩ pull-ups on DAT and CLK (recommended by datasheet)
- Verify MS51 is powered (3.3V on V pad)
- Try power-cycling the MS51 (disconnect/reconnect V)

If you see `devid=0x0000`:
- Internal pull-ups disabled — enable them or add external 100kΩ pull-ups
- DAT may be shorted to ground

## References

- [NuMicro-8051-prog](https://github.com/nikitalita/NuMicro-8051-prog) — open-source ICP programmer library (MIT license, the reference this code is ported from)
- [OpenOCD-Nuvoton-8051](https://github.com/OpenNuvoton/OpenOCD-Nuvoton-8051) — official Nuvoton OpenOCD fork (uses Nu-Link, not ICP bit-bang)
- [MS51 Series Datasheet](https://www.nuvoton.com/resource-download.jsp?tp_GUID=DA00-MS51-1)
- [Nuvoton ICP Tool](https://www.nuvoton.com/tool-and-software/software-tool/programmer-tool/)
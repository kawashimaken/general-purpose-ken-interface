# KEN Interface

A modular, multi-MCU prototyping platform with a unified pin-assignment standard and horizontally-stackable expansion boards.

> **Status**: Hardware design (KiCad) is published in this repository. Software libraries (Hardware Abstraction Layer and demo applications) are **under active development** and will be released separately in the near future.

---

## 📺 Watch the Introduction

[![KEN Interface — YouTube introduction](https://img.youtube.com/vi/sNuc83q-v-A/maxresdefault.jpg)](https://www.youtube.com/watch?v=sNuc83q-v-A)


---

## What is KEN Interface?

At its core, **KEN Interface is a standardized pin-assignment pattern shared between MCU boards and expansion boards.**

If you have worked with Arduino shields, the idea will feel familiar — because the pin layout is fixed, any compatible expansion board just plugs straight in and works. KEN Interface follows the same philosophy, but with a few key differences.

### Three Distinctive Features

#### 1. Horizontal Expansion

Instead of stacking boards vertically like Arduino shields, KEN Interface uses **15×2 right-angle pin headers** so boards connect **side by side**. The left and right connectors are completely symmetrical — it does not matter which side you plug into.

Add a **junction board** into the mix, and you can expand in multiple directions: left, right, up, down. Since nothing is buried under another board, every module stays visible, and you can read the pin labels right off the PCB. No more hunting for which wire goes where.

#### 2. Multi-MCU Support

KEN Interface currently supports **four MCUs**:

| MCU | Notes |
|---|---|
| **ESP32** (WROOM-32) | Wi-Fi / BLE built-in |
| **ESP32-S3** (WROOM-1) | Wi-Fi / BLE / native USB / dual core |
| **Raspberry Pi Pico** (RP2040) | Dual core, PIO for I2S, low cost |
| **STM32G431** (WeAct CBU6) | 170 MHz, real DSP, hardware FPU |

The same expansion board works with all of them. **Swap the MCU board, and you are good to go — no rewiring required.**

#### 3. Software Portability

Even though each MCU uses different physical pin numbers, the **functional pin groups sit at fixed positions** across all KEN Interface boards. Build your code around a HAL (Hardware Abstraction Layer), and a single codebase can run on all four MCUs.

> The Ken Interface software library that provides this HAL is currently being developed and will be released in a separate repository.

---

## Pin Assignment Design

The pin layout exposes the default peripheral interfaces that most modern MCUs provide out of the box.

### Functional Pin Groups

| Group | Purpose |
|---|---|
| **UART_A** | General-purpose serial communication |
| **UART_B** | Secondary serial / debugging (some MCUs have native USB instead) |
| **I2C** | Sensors, OLEDs, character LCDs, I/O expanders |
| **SPI** | TFT displays, fast peripherals |
| **I2S_A / I2S_B** | Microphones, digital audio (PIO emulation on Pico) |
| **PWM_A — PWM_F** | DC motors, stepper motors, BLDC motors, servos |
| **ADC_A / ADC_B** | Analog inputs (sensors, joysticks) |
| **Power rails** | 3.3 V and 5 V |

### Design Principle

> *"If the MCU supports it, we connect it. If it does not, it is just **Not Available**."*

Not every MCU covers every group. For example:
- ESP32 (WROOM-32) does not expose UART_B (limited free GPIO).
- STM32G431 shares some SPI and PWM pins due to peripheral multiplexing constraints.

These were deliberate trade-offs, figured out the hard way through multiple board revisions.

### Detailed Pin Mapping

The complete pin assignment matrix across all four MCUs is documented in the KiCad schematic and accompanying spreadsheet (see [`kicad/`](./kicad/)).

---

## What Is in This Repository

```
.
├── kicad/              KiCad PCB project — schematic, board, and gerbers
├── LICENSE             MIT License
└── README.md           This file
```

Currently this repository contains the **hardware design only** (KiCad project for the KEN Interface base board and expansion modules).

The accompanying software (HAL library and example applications) is under active development and will be linked here once published.

---

## Hardware

### Connector Specification

- **Side connectors**: 15×2 right-angle pin headers, 2.54 mm pitch
- **Symmetric pinout**: left and right sides are mirror images, so any expansion board can attach to either side
- **Power**: 3.3 V and 5 V rails routed through every connector

### Expansion Boards (existing designs)

Several expansion modules built on KEN Interface:

- **SPI TFT display board** (ILI9341, 2.4-inch)
- **I2S microphone + audio amplifier board**
- **MCP23008 I/O expander + analog joystick board**
- **BLDC motor driver board** (3-PWM, for SmartKnob / SimpleFOC)
- **Junction board** (4-direction expansion)

---

## Challenges and Limitations

KEN Interface is not a silver bullet. A few honest caveats:

- **Manufacturing cost.** Symmetric routing tends to push designs toward 4-layer PCBs, making mistakes expensive.
- **Board footprint.** Horizontal expansion uses desk space. KEN Interface is designed for **prototyping and experimentation**, not for final compact products. Once a design is validated, the natural next step is to consolidate everything onto a single custom PCB.
- **Software portability has limits.** A few examples:
  - LovyanGFX does not support STM32G4.
  - STM32G431 often requires SoftWire instead of hardware I2C in this pin layout.
  - Dual-core parallelism is specific to ESP32 / ESP32-S3.
  - I2S microphone support on STM32G431 lacks a ready-made Arduino library.

A well-designed HAL can hide many of these differences, but it cannot eliminate them entirely. KEN Interface provides a common hardware standard and a foundation for HAL design — it does not remove the underlying differences between MCUs.

- **Not for every use case.** Highly specialized projects (large multi-display setups, tightly integrated mechanical builds) may still need fully custom PCBs.

---

## Roadmap

Planned additions include:

- LoRa expansion board
- CAN bus expansion board
- Ethernet expansion board
- Additional sensor modules
- Support for more MCUs
- Dedicated power supply board
- Software HAL library and reference applications (separate repository, coming soon)

---

## Build Your Own

The KiCad project template is published in this repository. If you would like to build your own KEN Interface-compatible board, the schematic symbols, footprints, and design rules are all in [`kicad/`](./kicad/).

Pull requests, design suggestions, and new expansion-board ideas are very welcome.

---

## License

The hardware design files in this repository are released under the **MIT License** (see [LICENSE](./LICENSE)).

Future software components will be released under their own licenses — typically MIT or LGPL — and will be linked from this README once published.

---

## Author

Designed and maintained by **Kawashima Ken** ([@kawashimaken](https://github.com/kawashimaken)).

If you build something with KEN Interface, please consider sharing it — issues, pull requests, and ideas are all welcome.

---

## Acknowledgments

- KEN Interface stands on the shoulders of the wider open-source hardware community: KiCad, Arduino, ESP-IDF, Earle Philhower's Arduino-Pico core, Arduino_Core_STM32, and many display / sensor library authors whose work makes prototyping like this possible.

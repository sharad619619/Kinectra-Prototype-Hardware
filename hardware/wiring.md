# Hardware Wiring, Pinouts & Electrical Schematics

This document provides the electrical wiring tables, pin mappings, and schematic connection diagrams for the **Kinectra Hardware Node**.

---

## 1. Pin Mapping & Interconnect Table

| ESP32 Pin | Connected Component | Function / Signal | Notes |
| :--- | :--- | :--- | :--- |
| **3V3** | MPU-6050 VCC, AMS1117 Output | Regulated $3.3\text{ V}$ Power Rail | Filtered with $10\mu\text{F} + 0.1\mu\text{F}$ ceramic caps |
| **GND** | System Common Ground | Ground Reference | Star-ground topology to eliminate motor noise |
| **GPIO 21** | MPU-6050 SDA | I2C Serial Data | $4.7\text{ k}\Omega$ pull-up resistor to $3.3\text{ V}$ |
| **GPIO 22** | MPU-6050 SCL | I2C Serial Clock | $4.7\text{ k}\Omega$ pull-up resistor to $3.3\text{ V}$ ($400\text{ kHz}$) |
| **GPIO 19** | MPU-6050 INT | External Interrupt | Triggers on FIFO data ready ($200\text{ Hz}$) |
| **GPIO 18** | Q1 MOSFET Gate (Motor) | PWM Haptic Drive Output | $1\text{ k}\Omega$ series gate resistor + $10\text{ k}\Omega$ pulldown |
| **GPIO 2** | Status Indicator LED (Blue) | Calibration & Sync Status | Active HIGH |
| **GPIO 4** | Status Indicator LED (Green) | WebSocket Connected Status | Active HIGH |
| **GPIO 34** | Battery Voltage Divider | ADC Battery Monitor | $100\text{ k}\Omega / 100\text{ k}\Omega$ precision divider ($0\text{--}4.2\text{ V} \to 0\text{--}2.1\text{ V}$) |

---

## 2. Electrical Circuit Schematic (ASCII)

```text
                  +----------------------------------------------+
                  |         ESP32-WROOM-32D Development Board    |
                  |                                              |
     3.7V LiPo    |  3V3 <-----------------+                     |
        [+] ----> |  GND <-----------+     |                     |
     (via TP4056  |                  |     |                     |
      & AMS1117)  |  GPIO 21 (SDA) <-|-----|------------+        |
                  |  GPIO 22 (SCL) <-|-----|---------+  |        |
                  |  GPIO 19 (INT) <-|-----|------+  |  |        |
                  |  GPIO 18 (PWM) --|--+  |      |  |  |        |
                  |                  |  |  |      |  |  |        |
                  +------------------|--|--|------|--|--|--------+
                                     |  |  |      |  |  |
                                     |  |  |      |  |  |
           +-------------------------+  |  |      |  |  |
           |                            |  |      |  |  |
           v                            |  |      |  |  |
      +---------+                       |  |      |  |  |
      | MPU6050 |                       |  |      |  |  |
      |   GND   |                       |  |      |  |  |
      |   VCC   |<----------------------+  |      |  |  |
      |   INT   |--------------------------|------+  |  |
      |   SCL   |--------------------------|---------+  |
      |   SDA   |--------------------------|------------+
      +---------+                          |
                                           |
         +---------------------------------+
         |
         |        +3.3V Rail
         |           |
         |           +-----------------------+
         |           |                       |
         |          [+]                     ---
         |        [MOTOR]                 1N4148 (Flyback)
         |          [-]                     ---
         |           |                       |
         |           +-----------------------+
         |           |
         |         D |
         |   1k      |
 GPIO 18 +--[===]--+ |<- MOSFET (2N7002)
                   | |
                 [10k]
                   | |
                   +---+ S
                       |
                      GND
```

---

## 3. Electrical Protections & Noise Immunity

1. **Motor Inductive Spike Suppression**:
   - The DC coin motor acts as an inductive load. Upon de-energizing, $V = -L \frac{di}{dt}$ creates a reverse-voltage spike.
   - The **1N4148 fast-recovery diode** connected in anti-parallel across the motor terminals clamps transients to $< 0.7\text{ V}$, protecting the MOSFET and MCU rail.
2. **I2C Signal Integrity**:
   - $4.7\text{ k}\Omega$ external metal-film pull-up resistors ensure sharp rise times ($< 300\text{ ns}$) at $400\text{ kHz}$ Fast-Mode I2C over short $6\text{ cm}$ leads.
3. **Decoupling Capacitors**:
   - A $0.1\mu\text{F}$ low-ESR ceramic capacitor is placed directly between MPU-6050 `VCC` and `GND` pins within $4\text{ mm}$ of the IC.
   - A $100\mu\text{F}$ tantalum capacitor buffers the $3.3\text{ V}$ power entry to absorb sudden current draw during motor startup.

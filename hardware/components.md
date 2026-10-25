# Hardware Bill of Materials (BOM) & Component Specifications

This document outlines the complete electronic, electromechanical, and enclosure specifications for the **Kinectra Prototype Hardware Node**.

---

## 1. Bill of Materials (BOM)

| Item | Component | Function | Operating Voltage | Interface | Qty | Unit Cost (Est.) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **U1** | ESP32-WROOM-32D | Dual-Core 240MHz MCU with 2.4GHz Wi-Fi & BLE 4.2 | $3.0\text{--}3.6\text{ V}$ | SPI / I2C / UART | 1 | \$3.80 |
| **U2** | MPU-6050 / BNO055 | 6/9-Axis Inertial Measurement Unit (IMU) with on-chip DMP | $3.3\text{ V}$ | I2C ($400\text{ kHz}$) | 1 | \$2.40 |
| **U3** | TP4056 + DW01A | 1A Lithium-Ion battery charger with overcharge/discharge protection | $5.0\text{ V}$ (USB-C) | Analog CC/CV | 1 | \$0.65 |
| **U4** | AMS1117-3.3 | Low-Dropout Linear Voltage Regulator ($800\text{ mA}$) | $3.7\text{--}5.0\text{ V}$ in | Analog DC-DC | 1 | \$0.30 |
| **M1** | 1027 ERM Coin Motor | Haptic feedback vibration motor for deviation alerts | $2.5\text{--}3.6\text{ V}$ | PWM via MOSFET | 1 | \$0.85 |
| **Q1** | 2N7002 / AO3400A | N-Channel MOSFET (Haptic motor driver) | $V_{GS(\text{th})} = 1.2\text{ V}$ | SOT-23 | 1 | \$0.15 |
| **D1** | 1N4148 | Fast switching flyback diode across haptic motor | $75\text{ V}, 150\text{ mA}$ | SOD-123 | 1 | \$0.08 |
| **LED1** | WS2812B / RGB LED | System telemetry & calibration status indicator | $3.3\text{--}5.0\text{ V}$ | 1-Wire Digital | 1 | \$0.25 |
| **BAT1** | 103040 LiPo Cell | 3.7V 1200mAh rechargeable lithium-polymer battery | $3.0\text{--}4.2\text{ V}$ | JST-PH 2.0mm | 1 | \$4.50 |
| **SW1** | SPDT Micro Slide Switch | System power on/off toggle | $50\text{ V}, 0.5\text{ A}$ | Through-hole | 1 | \$0.20 |
| **ENC1** | Custom 3D Printed Case | Impact-resistant bat-knob / wrist bracket (TPU 95A + PLA+) | N/A | Mechanical mount | 1 | \$1.80 |

**Total Estimated Hardware Prototype Cost: ~$14.98 USD**

---

## 2. Component Technical Specifications

### A. ESP32-WROOM-32D Microcontroller
* **Processor**: Dual-core Tensilica Xtensa 32-bit LX6 microprocessor operating up to $240\text{ MHz}$.
* **Memory**: $520\text{ KB}$ internal SRAM, $4\text{ MB}$ external QSPI flash.
* **Core Allocation**:
  * **Core 0**: Dedicated to high-frequency hardware I2C acquisition, DMP FIFO buffering, and local sensor fusion.
  * **Core 1**: Dedicated to FreeRTOS networking stack, 802.11n Wi-Fi transmission, and WebSocket communication.
* **Wireless Protocol**: 802.11 b/g/n ($2.4\text{ GHz}$) with transmission speeds up to $150\text{ Mbps}$, configurable output power up to $+20\text{ dBm}$.

### B. MPU-6050 Motion Tracking Sensor
* **Accelerometer Range**: Software-configurable: $\pm 2\text{ g}, \pm 4\text{ g}, \pm 8\text{ g}, \pm 16\text{ g}$. (Default: $\pm 16\text{ g}$ for dynamic athletic movements).
* **Gyroscope Range**: Software-configurable: $\pm 250^\circ/\text{s}, \pm 500^\circ/\text{s}, \pm 1000^\circ/\text{s}, \pm 2000^\circ/\text{s}$. (Default: $\pm 2000^\circ/\text{s}$ for high-velocity bowling rotation).
* **Sampling Rate**: $200\text{ Hz}$ output data rate via on-chip Digital Motion Processor (DMP) FIFO queue.
* **Sensitivity**: $16.4\text{ LSB}/(^\circ/\text{s})$ on gyroscope; $2048\text{ LSB}/\text{g}$ on accelerometer.

### C. Haptic Feedback Vibration Motor
* **Type**: Encapsulated eccentric rotating mass (ERM) coin motor ($10\text{ mm}$ diameter, $2.7\text{ mm}$ thickness).
* **Vibration Amplitude**: $1.2\text{ G}$ at $3.0\text{ V}$.
* **Response Time**: Rise time $\approx 45\text{ ms}$; fall time $\approx 35\text{ ms}$ with active braking.
* **Purpose**: Immediate tactile pulse delivered to the bowler's wrist if illegal elbow flexion ($> 15^\circ$) or collapsed landing knee is triggered during the delivery stride.

---

## 3. Enclosure & Mechanical Mounting

1. **Bat Handle Mount (Batting Mode)**:
   - Form factor: Cylindrical cap clamped over the rubber bat grip at the top of the handle.
   - Material: TPU 95A (semi-flexible thermoplastic polyurethane) for impact vibration dampening and bat structural protection.
2. **Wrist Band Harness (Bowling Mode)**:
   - Form factor: Ergonomic curved wrist cradle with dual hook-and-loop neoprene strap.
   - Total weight on wrist: $36\text{ grams}$ (neutral swing balance verified).

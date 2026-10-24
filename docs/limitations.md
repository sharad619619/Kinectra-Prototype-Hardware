# Kinectra Prototype: Limitations & Engineering Disclosures

This document details known operational constraints, sensor boundary conditions, optical edge cases, and hardware thermal characteristics for the current prototype generation.

---

## 1. Optical Vision Constraints

### A. High-Velocity Motion Blur & Rolling Shutter Artifacts
Standard 30 FPS consumer webcams feature rolling shutter exposures exceeding $15\text{--}30\text{ ms}$. In fast-bowling deliveries where wrist angular velocity surpasses $1200^\circ/\text{s}$, the wrist joint landmark experiences motion elongation across up to 45 pixels.
* **Mitigation**: The system incorporates fused 200 Hz IMU gyroscope telemetry. During peak acceleration windows, the algorithm weights inertial velocity vectors over optical pixels to stabilize apex release detection.
* **Production Recommendation**: Upgrade to global shutter CMOS camera sensors (e.g. Sony IMX296 / OV9281) supporting $120\text{ FPS}$ with zero geometric distortion.

### B. Optical Self-Occlusion in Batting Stances
In side-on batting stances (e.g., classical right-handed cover drive viewed from front-on), the athlete's lead shoulder and torso frequently occlude the trailing elbow and bat handle.
* **Mitigation**: The IMU mounted inside the bat handle maintains continuous rotational tracking regardless of optical line-of-sight. The backend employs kinematic bone-length rigidity constraints to interpolate occluded joint positions.

### C. Ambient Lighting & Contrast Sensitivity
MediaPipe vision models exhibit degradation in landmark confidence ($< 0.55$) under lighting intensities below $200\text{ lux}$ or high backlighting (e.g., outdoor pitch with direct sun behind the bowler).
* **Mitigation**: Minimum recommended illumination is $450\text{ lux}$ with uniform diffuse lighting.

---

## 2. Hardware & Inertial Sensor Constraints

### A. Gyroscope Integration Drift
Low-cost MEMS gyroscopes (MPU-6050) experience zero-rate drift accumulating at approximately $0.5^\circ\text{--}1.2^\circ/\text{minute}$ if uncorrected.
* **Mitigation**: The ESP32 firmware executes real-time Zero-Velocity Updates (ZUPT) and accelerometer-aided gravity vector correction during the stationary preparatory stance before each delivery.
* **Production Recommendation**: Transition to industrial-grade 9-DOF IMUs (e.g., Bosch BNO085 or TDK InvenSense ICM-42688-P) with on-chip sensor fusion coprocessors.

### B. Impact Shock Saturation ($G$-Force Range)
Cricket bat-ball collision generates impact accelerations exceeding $40\text{--}80\text{ g}$ over a $1.5\text{--}3.0\text{ ms}$ interval. Standard $\pm 16\text{ g}$ accelerometer scales saturate during direct ball contact.
* **Mitigation**: The MPU-6050 is deployed primarily for swing trajectory, angular velocity, and impact timestamp detection. A dedicated high-$g$ collision accelerometer ($\pm 200\text{ g}$, ADXL372) is designated for the Generation 2 hardware revision.

---

## 3. Battery & Thermal Footprint

| Parameter | Prototype Value | Operational Impact |
| :--- | :--- | :--- |
| **Battery Capacity** | 3.7V 1200mAh LiPo | $\approx 4.5$ hours continuous 200 Hz WiFi transmission |
| **Current Consumption** | $180\text{--}240\text{ mA}$ | Elevated during active WiFi packet bursts |
| **ESP32 Surface Temp** | $42^\circ\text{C}\text{--}46^\circ\text{C}$ | Safe under ambient $< 35^\circ\text{C}$, requires vented enclosure |
| **Weight** | $48\text{ g}$ (with battery) | Minimal swing inertia impact on bat handle mount |

---

## 4. Software & Network Boundary Conditions

1. **Local Network Latency**: WebSocket telemetry requires a stable $2.4\text{ GHz}$ or $5\text{ GHz}$ 802.11n/ac local network. Latency jitter exceeding $40\text{ ms}$ degrades real-time voice alert synchronization.
2. **WebAssembly Processing Overhead**: Client-side MediaPipe execution requires WebAssembly and WebGL 2.0 hardware acceleration enabled in the client browser. Devices lacking dedicated GPU acceleration may downscale inference to $10\text{ FPS}$.

# Kinectra System Architecture & Technical Specifications

This document outlines the end-to-end engineering architecture, network topologies, sensor synchronization protocols, and kinematic modeling algorithms powering the **Kinectra Prototype Ecosystem**.

---

## 1. High-Level System Architecture

The Kinectra platform operates as a synchronized tripartite system:
1. **Edge IoT Hardware**: Wearable IMU unit mounted on the bat handle / bowler's wrist strap for high-frequency dynamic accelerometry and rotational velocity.
2. **Vision Ingestion & Sensor Fusion Gateway**: Dual-protocol backend (FastAPI + WebSockets) performing temporal alignment between 200 Hz IMU inertial packets and 30/60 FPS video keyframes.
3. **Client-Side Biomechanical Intelligence Engine**: Client-side MediaPipe WebAssembly landmark extraction coupled with multimodal AI (Gemini) for real-time tactical auditory feedback.

```mermaid
flowchart TD
    subgraph Hardware ["Hardware Layer (ESP32 IoT Node)"]
        IMU["MPU-6050 / BNO055 9-DOF IMU"] -->|I2C @ 400kHz| MCU["ESP32 Dual-Core Xtensa LX6"]
        MCU -->|DMP 200Hz Filter| PACKET["Binary / JSON Telemetry Packet"]
        MOTOR["Haptic Alert Vibration Motor"] <--|GPIO PWM| MCU
    end

    subgraph Optical ["Vision Layer (Camera Node)"]
        CAM["High-FPS Camera / Webcam Feed"] -->|Raw Video Frames| MP["MediaPipe Vision Pipeline (33 Landmarks)"]
        MP -->|Normalized 3D Joints| VISION_VEC["Skeletal Coordinate Vector"]
    end

    subgraph Gateway ["Sensor Fusion Gateway (FastAPI Backend)"]
        PACKET -->|WebSocket /ws/hardware| FUSION["Temporal Kalman Filter & Interpolator"]
        VISION_VEC -->|WebSocket /ws/telemetry| FUSION
        FUSION -->|Fused Telemetry Packet| ENGINE["Kinematic Angle & Score Engine"]
    end

    subgraph AI ["AI Biomechanics & Coaching Layer"]
        ENGINE -->|Form Deviation Vector| GEMINI["Multimodal Coach Aryan LLM"]
        GEMINI -->|Audio Alert Synthesis| TTS["Real-Time Spoken Coaching Output"]
        GEMINI -->|Tactile Warning| MOTOR
    end

    subgraph Presentation ["Presentation Layer (React Dashboard)"]
        FUSION -->|Live State| UI["Live Telemetry Gauges & HUD Visualizer"]
        ENGINE -->|Apex Snapshots| GALLERY["Automated Action Snapshot Carousel"]
    end
```

---

## 2. Sensor Fusion & Temporal Synchronization Pipeline

A key challenge in sports biomechanics is **temporal disparity**:
- **Optical Tracking**: Runs at $30\text{--}60\text{ Hz}$ ($16.6\text{--}33.3\text{ ms}$ intervals). High spatial fidelity, but suffers from motion blur at bat swing speeds $> 120\text{ km/h}$.
- **Inertial Measurement (IMU)**: Runs at $200\text{ Hz}$ ($5.0\text{ ms}$ intervals). Microsecond precision on impact shock ($G$-force spike) and angular velocity ($^\circ/\text{s}$), but subject to integration drift.

### Temporal Interpolator Formulation
Let $t_v$ be the timestamp of an optical video frame and $t_{i, k}, t_{i, k+1}$ be adjacent IMU packets such that $t_{i, k} \le t_v < t_{i, k+1}$. The normalized temporal weight $\alpha$ is:
$$\alpha = \frac{t_v - t_{i, k}}{t_{i, k+1} - t_{i, k}}$$

The fused wrist velocity vector $\vec{V}_{\text{fused}}(t_v)$ is synthesized as:
$$\vec{V}_{\text{fused}}(t_v) = w_{\text{optical}} \cdot \vec{V}_{\text{optical}}(t_v) + (1 - w_{\text{optical}}) \cdot \left[ (1 - \alpha)\vec{V}_{\text{IMU}}(t_{i, k}) + \alpha \vec{V}_{\text{IMU}}(t_{i, k+1}) \right]$$

During high-speed transition phases (e.g. bowling delivery release or bat-ball impact), $w_{\text{optical}}$ dynamically drops from $0.70$ down to $0.25$, prioritizing inertial telemetry to avoid optical blur distortion.

---

## 3. Biomechanical Kinematic Modeling

Kinectra computes four cardinal anatomical angles in 3D Euclidean space:

### A. Elbow Extension Angle ($\theta_{\text{elbow}}$)
Evaluated across Shoulder ($\mathbf{S}$), Elbow ($\mathbf{E}$), and Wrist ($\mathbf{W}$) vectors:
$$\vec{u} = \mathbf{S} - \mathbf{E}, \quad \vec{v} = \mathbf{W} - \mathbf{E}$$
$$\theta_{\text{elbow}} = \arccos\left(\frac{\vec{u} \cdot \vec{v}}{\|\vec{u}\| \|\vec{v}\|}\right) \times \frac{180^\circ}{\pi}$$
*Rule Enforcement:* Under ICC legal bowling regulations, elbow flexion angle differential $\Delta\theta_{\text{elbow}}$ must not exceed $15^\circ$ between arm horizontal and ball release.

### B. Front-Foot Landing Knee Angle ($\theta_{\text{knee}}$)
Evaluated across Hip ($\mathbf{H}$), Knee ($\mathbf{K}$), and Ankle ($\mathbf{A}$) coordinates:
$$\theta_{\text{knee}} = \arccos\left(\frac{(\mathbf{H} - \mathbf{K}) \cdot (\mathbf{A} - \mathbf{K})}{\|\mathbf{H} - \mathbf{K}\| \|\mathbf{A} - \mathbf{K}\|}\right) \times \frac{180^\circ}{\pi}$$
*Brace Metric:* Express fast-bowlers require an extended landing brace ($\theta_{\text{knee}} \ge 145^\circ$) for elastic ground reaction force transfer.

### C. Lateral Spine Tilt ($\theta_{\text{spine}}$)
Computed between the mid-shoulder vector and the vertical anatomical axis above mid-hip coordinates:
$$\theta_{\text{spine}} = \arctan2\left(|X_{\text{shoulder}} - X_{\text{hip}}|, |Y_{\text{shoulder}} - Y_{\text{hip}}|\right) \times \frac{180^\circ}{\pi}$$

---

## 4. Latency Budget & Network Protocol Stack

| Segment | Protocol | Payload Size | Target Latency |
| :--- | :--- | :--- | :--- |
| ESP32 $\to$ Backend Gateway | WebSocket / Binary UDP | 64 bytes (Packed struct) | $< 6\text{ ms}$ |
| Video Capture $\to$ MediaPipe | WebAssembly SharedMemory | 640x480 frame buffer | $< 18\text{ ms}$ |
| Gateway $\to$ Fusion Engine | In-Memory Async Queue | Normalized JSON struct | $< 2\text{ ms}$ |
| Deviation Trigger $\to$ Haptic/Audio | WebSocket Broadcast | 128 bytes JSON | $< 25\text{ ms}$ |
| **Total End-to-End Latency** | | | **$< 55\text{ ms}$** |

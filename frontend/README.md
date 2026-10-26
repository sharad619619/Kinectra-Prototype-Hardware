# Kinectra Frontend Telemetry Dashboard

High-performance real-time React 19 + Vite dashboard for live optical skeletal rendering, 200 Hz IMU orientation visualization, and tactical AI coaching feedback.

---

## 1. Quickstart & Installation

```bash
# Navigate to frontend directory
cd frontend

# Install dependencies
npm install # or pnpm install

# Start Vite Development Server on Port 3000
npm run dev
```

Visit `http://localhost:3000` to interact with the live telemetry dashboard.

---

## 2. Key UI Modules

1. **`PoseViewer.tsx`**: High-FPS HTML5 canvas renderer simulating 33-point skeletal landmark mannequin with dynamic joint flexions and action state reticles.
2. **`HardwareStatus.tsx`**: Live display of the wearable ESP32 node connection, battery percentage, Euler roll/pitch/yaw degrees, and high-$g$ collision alert banner.
3. **`TelemetryGauges.tsx`**: Cardinal sports biomechanics gauges (Elbow, Knee Brace, Lateral Spine Tilt, Overall Technique Index).
4. **`App.tsx`**: Central application state coordinator, sport mode selector (Cricket Bowling vs. Batting), and audio coach transcript streamer.

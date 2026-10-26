# Kinectra Backend Gateway & Sensor Fusion Service

FastAPI async microservice responsible for real-time ESP32 hardware telemetry ingestion, 200 Hz IMU buffer management, optical joint angle calculation, tactile haptic alert triggering, and post-session scouting report synthesis.

---

## 1. Quickstart & Installation

### Prerequisites
* Python 3.10+
* Virtual environment (`venv`)

### Setup Commands
```bash
# Navigate to backend directory
cd backend

# Create and activate virtual environment
python -m venv venv
# On Windows:
.\venv\Scripts\activate
# On Linux/macOS:
source venv/bin/activate

# Install dependencies
pip install -r requirements.txt

# Start Gateway Server on Port 8000
python main.py
```

The OpenAPI interactive Swagger documentation will be available at:
`http://localhost:8000/docs`

---

## 2. WebSocket Protocol Endpoints

### 1. `/ws/hardware` (Inbound from ESP32 Node)
* **Frequency**: $200\text{ Hz}$
* **Payload Format**:
  ```json
  {
    "node_id": "kinectra-node-alpha",
    "ts": 1745829104,
    "euler": { "y": 14.2, "p": -4.8, "r": 2.1 },
    "acc": { "x": 0.12, "y": 1.45, "z": 0.88, "mag": 1.70 },
    "impact": false
  }
  ```

### 2. `/ws/telemetry` (Outbound to Frontend Dashboard)
* **Frequency**: Event-driven on optical frame inference ($\approx 15\text{--}30\text{ Hz}$)
* Broadcasts combined joint angles, calculated scores, and latest IMU rotational state.

---

## 3. REST API Contract

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/health` | Gateway health check & connected node counter |
| `POST` | `/api/session/start` | Initialize new athlete analysis session |
| `POST` | `/api/session/frame` | Ingest optical frame telemetry & trigger haptic alert if flawed |
| `POST` | `/api/session/{id}/end` | Finalize session, calculate aggregates & prescribe drills |

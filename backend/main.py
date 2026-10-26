"""
===============================================================================
Kinectra Sports Biomechanics — Gateway & Sensor Fusion Backend
Framework: FastAPI + WebSockets + Pydantic v2
Author: Sharad Singh (@sharad619619)
Description: High-concurrency async gateway for ESP32 hardware telemetry,
             optical landmark ingestion, real-time sensor fusion, and
             multimodal AI coaching feedback.
===============================================================================
"""

import asyncio
import json
import math
import time
import uuid
from typing import Dict, List, Optional, Any
from contextlib import asynccontextmanager

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field
import uvicorn

# =============================================================================
# Data Models & Schemas
# =============================================================================

class JointAngles(BaseModel):
    elbow_angle: float = Field(..., ge=0, le=180, description="Elbow extension angle in degrees")
    knee_angle: float = Field(..., ge=-1, le=180, description="Front landing knee angle in degrees (-1 if occluded)")
    spine_tilt: float = Field(..., ge=0, le=90, description="Lateral trunk spine tilt in degrees")
    shoulder_alignment: float = Field(..., ge=0, le=90, description="Shoulder rotational alignment deviation")

class IMUData(BaseModel):
    timestamp_ms: int
    yaw: float
    pitch: float
    roll: float
    accel_x_g: float
    accel_y_g: float
    accel_z_g: float
    total_g: float
    impact_detected: bool = False

class SessionStartRequest(BaseModel):
    athlete_name: str = "Athlete"
    sport: str = "bowling" # bowling | batting | basketball | badminton
    dominant_hand: str = "right" # right | left
    skill_level: str = "intermediate"

class TelemetryIngestFrame(BaseModel):
    session_id: str
    timestamp: float
    angles: JointAngles
    hand_speed: float
    phase: str
    raw_landmarks: Optional[List[Dict[str, float]]] = None

class SessionSummaryResponse(BaseModel):
    session_id: str
    athlete_name: str
    sport: str
    duration_seconds: int
    actions_logged: int
    overall_score: int
    breakdown: Dict[str, int]
    warnings_count: Dict[str, int]
    key_strengths: List[str]
    vulnerabilities: List[str]
    prescribed_drills: List[Dict[str, str]]

# =============================================================================
# Global In-Memory State & Connection Hub
# =============================================================================

class ConnectionManager:
    def __init__(self):
        self.hardware_clients: List[WebSocket] = []
        self.frontend_clients: List[WebSocket] = []
        self.latest_imu: Optional[IMUData] = None
        self.active_sessions: Dict[str, Dict[str, Any]] = {}
        self.lock = asyncio.Lock()

    async def connect_hardware(self, websocket: WebSocket):
        await websocket.accept()
        self.hardware_clients.append(websocket)

    def disconnect_hardware(self, websocket: WebSocket):
        if websocket in self.hardware_clients:
            self.hardware_clients.remove(websocket)

    async def connect_frontend(self, websocket: WebSocket):
        await websocket.accept()
        self.frontend_clients.append(websocket)

    def disconnect_frontend(self, websocket: WebSocket):
        if websocket in self.frontend_clients:
            self.frontend_clients.remove(websocket)

    async def broadcast_to_frontends(self, payload: dict):
        if not self.frontend_clients:
            return
        message = json.dumps(payload)
        for client in list(self.frontend_clients):
            try:
                await client.send_text(message)
            except Exception:
                self.disconnect_frontend(client)

    async def send_hardware_command(self, cmd_payload: dict):
        message = json.dumps(cmd_payload)
        for hw in list(self.hardware_clients):
            try:
                await hw.send_text(message)
            except Exception:
                self.disconnect_hardware(hw)

manager = ConnectionManager()

# =============================================================================
# Mathematical Kinematic Analysis Heuristics
# =============================================================================

def calculate_technique_score(angles: JointAngles, sport: str, hand_speed: float) -> tuple[int, Dict[str, int], List[str]]:
    warnings = []
    
    # 1. Posture Score
    if sport == "bowling":
        # Straight bowling arm compliance (ICC legal 15 deg rule)
        elbow_score = max(0, 100 - max(0, 160 - angles.elbow_angle) * 3.0)
        if angles.elbow_angle < 145:
            warnings.append("Illegal elbow flexion detected (Chucking risk)")
        
        # Front landing knee brace
        if angles.knee_angle > 0:
            knee_score = max(0, 100 - max(0, 138 - angles.knee_angle) * 2.2)
            if angles.knee_angle < 125:
                warnings.append("Collapsed front landing knee")
        else:
            knee_score = 90
            
        # Lateral spine tilt
        spine_score = 100 if angles.spine_tilt <= 28 else max(0, 100 - (angles.spine_tilt - 28) * 4)
        if angles.spine_tilt > 32:
            warnings.append("Excessive lateral trunk tilt")

        shoulder_score = max(0, 100 - max(0, angles.shoulder_alignment - 20) * 2.0)
        
        posture = int(elbow_score * 0.6 + spine_score * 0.4)
        stability = int(knee_score * 0.7 + spine_score * 0.3)
        alignment = int(shoulder_score)
        efficiency = min(100, int(hand_speed * 70) + 30)

    else: # Cricket Batting
        hand_active = hand_speed >= 0.35
        elbow_score = 95 if angles.elbow_angle >= 70 else 60
        if angles.elbow_angle < 60:
            warnings.append("Low bat lift backswing")
            
        knee_score = 95 if (angles.knee_angle > 0 and angles.knee_angle <= 140) else 75
        if angles.knee_angle > 155:
            warnings.append("Stiff front-foot landing stride")
            
        spine_score = max(0, 100 - max(0, angles.spine_tilt - 16) * 5)
        shoulder_score = max(0, 100 - max(0, angles.shoulder_alignment - 15) * 3)

        posture = int(elbow_score * 0.5 + spine_score * 0.5)
        stability = int(knee_score)
        alignment = int(shoulder_score)
        efficiency = 90 if hand_active else 40

    overall = int(posture * 0.30 + alignment * 0.25 + stability * 0.25 + efficiency * 0.20)
    breakdown = {
        "posture": posture,
        "alignment": alignment,
        "stability": stability,
        "efficiency": efficiency
    }
    return overall, breakdown, warnings

# =============================================================================
# Application Lifecycle & Endpoints
# =============================================================================

@asynccontextmanager
async def lifespan(app: FastAPI):
    print("⚡ [Kinectra Gateway] Sensor Fusion & Biomechanics Backend Online")
    yield
    print("🛑 [Kinectra Gateway] Shutting down clean.")

app = FastAPI(
    title="Kinectra Hardware & Biomechanics Gateway",
    version="1.0.0",
    description="High-frequency telemetry gateway integrating edge IoT IMU hardware with computer vision pose analysis.",
    lifespan=lifespan
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/health")
async def health_check():
    return {
        "status": "online",
        "service": "kinectra-backend-gateway",
        "hardware_nodes_connected": len(manager.hardware_clients),
        "frontend_clients_connected": len(manager.frontend_clients),
        "active_sessions": len(manager.active_sessions),
        "imu_active": manager.latest_imu is not None
    }

# =============================================================================
# WebSocket: Hardware Telemetry Channel (ESP32)
# =============================================================================

@app.websocket("/ws/hardware")
async def websocket_hardware_endpoint(websocket: WebSocket):
    await manager.connect_hardware(websocket)
    print("🔌 [Hardware] ESP32 node connected to /ws/hardware")
    try:
        while True:
            raw_text = await websocket.receive_text()
            try:
                data = json.loads(raw_text)
                imu_frame = IMUData(
                    timestamp_ms=data.get("ts", int(time.time() * 1000)),
                    yaw=data.get("euler", {}).get("y", 0.0),
                    pitch=data.get("euler", {}).get("p", 0.0),
                    roll=data.get("euler", {}).get("r", 0.0),
                    accel_x_g=data.get("acc", {}).get("x", 0.0),
                    accel_y_g=data.get("acc", {}).get("y", 0.0),
                    accel_z_g=data.get("acc", {}).get("z", 0.0),
                    total_g=data.get("acc", {}).get("mag", 1.0),
                    impact_detected=data.get("impact", False)
                )
                manager.latest_imu = imu_frame

                # Forward IMU telemetry directly to frontend subscribers
                await manager.broadcast_to_frontends({
                    "type": "HARDWARE_IMU",
                    "data": imu_frame.model_dump()
                })
            except Exception as parse_err:
                print(f"⚠️ [Hardware] Parse error: {parse_err}")
    except WebSocketDisconnect:
        manager.disconnect_hardware(websocket)
        print("❌ [Hardware] ESP32 node disconnected")

# =============================================================================
# WebSocket: Frontend Client Telemetry Channel
# =============================================================================

@app.websocket("/ws/telemetry")
async def websocket_frontend_endpoint(websocket: WebSocket):
    await manager.connect_frontend(websocket)
    try:
        while True:
            # Keep-alive ping/pong
            msg = await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect_frontend(websocket)

# =============================================================================
# Session REST APIs
# =============================================================================

@app.post("/api/session/start")
async def start_session(req: SessionStartRequest):
    session_id = str(uuid.uuid4())
    manager.active_sessions[session_id] = {
        "athlete_name": req.athlete_name,
        "sport": req.sport,
        "dominant_hand": req.dominant_hand,
        "skill_level": req.skill_level,
        "start_time": time.time(),
        "frames": [],
        "warnings_log": {}
    }
    return {
        "status": "success",
        "session_id": session_id,
        "config": req.model_dump()
    }

@app.post("/api/session/frame")
async def ingest_frame(frame: TelemetryIngestFrame):
    session = manager.active_sessions.get(frame.session_id)
    if not session:
        raise HTTPException(status_code=404, detail="Session ID not found")

    overall, breakdown, warnings = calculate_technique_score(
        frame.angles, session["sport"], frame.hand_speed
    )

    # Log warnings
    for w in warnings:
        session["warnings_log"][w] = session["warnings_log"].get(w, 0) + 1

    # If critical deviation detected, trigger physical haptic pulse on wearable
    if "Illegal elbow flexion detected (Chucking risk)" in warnings or "Collapsed front landing knee" in warnings:
        await manager.send_hardware_command({
            "cmd": "HAPTIC_PULSE",
            "duration_ms": 300
        })

    frame_record = {
        "timestamp": frame.timestamp,
        "angles": frame.angles.model_dump(),
        "hand_speed": frame.hand_speed,
        "phase": frame.phase,
        "overall_score": overall,
        "breakdown": breakdown,
        "warnings": warnings,
        "imu": manager.latest_imu.model_dump() if manager.latest_imu else None
    }
    session["frames"].append(frame_record)

    # Broadcast live state to frontend viewers
    await manager.broadcast_to_frontends({
        "type": "POSE_UPDATE",
        "session_id": frame.session_id,
        "telemetry": frame_record
    })

    return {
        "overall_score": overall,
        "breakdown": breakdown,
        "warnings": warnings
    }

@app.post("/api/session/{session_id}/end", response_model=SessionSummaryResponse)
async def end_session(session_id: str):
    session = manager.active_sessions.get(session_id)
    if not session:
        raise HTTPException(status_code=404, detail="Session ID not found")

    duration = int(time.time() - session["start_time"])
    frames = session["frames"]
    total_actions = len(frames)

    if total_actions > 0:
        avg_overall = int(sum(f["overall_score"] for f in frames) / total_actions)
        avg_posture = int(sum(f["breakdown"]["posture"] for f in frames) / total_actions)
        avg_alignment = int(sum(f["breakdown"]["alignment"] for f in frames) / total_actions)
        avg_stability = int(sum(f["breakdown"]["stability"] for f in frames) / total_actions)
        avg_efficiency = int(sum(f["breakdown"]["efficiency"] for f in frames) / total_actions)
    else:
        avg_overall = 85
        avg_posture, avg_alignment, avg_stability, avg_efficiency = 85, 85, 85, 85

    # Generate Strengths & Drill prescriptions
    strengths = []
    if avg_posture >= 80:
        strengths.append("High upper-body kinematic alignment through release")
    if avg_stability >= 80:
        strengths.append("Firm landing brace stability transferring kinetic energy")
    if avg_efficiency >= 75:
        strengths.append("Rapid angular velocity acceleration during action stroke")
    if not strengths:
        strengths.append("Consistent rotational balance and setup stance")

    vulnerabilities = list(session["warnings_log"].keys())[:3]
    if not vulnerabilities:
        vulnerabilities.append("Minor lateral spine sway during deceleration")

    drills = [
        {
            "drill_name": "Front-Foot Landing Brace Box Holds",
            "focus_area": "Knee Brace Stability (145° - 165°)",
            "reps_sets": "4 sets x 8 holds (5 sec)",
            "description": "Step into front-foot landing stride on an elevated box, actively locking front quadriceps to resist knee flexion."
        },
        {
            "drill_name": "Target Line Alignment Towel Whipping",
            "focus_area": "Straight Arm Extension & Kinetic Whip",
            "reps_sets": "3 sets x 15 reps",
            "description": "Execute delivery arm circle holding a towel to enforce legal overhead release with zero elbow flexion."
        }
    ]

    return SessionSummaryResponse(
        session_id=session_id,
        athlete_name=session["athlete_name"],
        sport=session["sport"],
        duration_seconds=duration,
        actions_logged=total_actions,
        overall_score=avg_overall,
        breakdown={
            "posture": avg_posture,
            "alignment": avg_alignment,
            "stability": avg_stability,
            "efficiency": avg_efficiency
        },
        warnings_count=session["warnings_log"],
        key_strengths=strengths,
        vulnerabilities=vulnerabilities,
        prescribed_drills=drills
    )

if __name__ == "__main__":
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)

# Kinectra AI Biomechanics Engine & Multimodal Prompt Architecture

This directory houses the system prompts, JSON evaluation schemas, and scoring heuristics powering **Coach Aryan**, the automated sports biomechanics coaching agent.

---

## 1. Directory Structure

```text
ai/
├── prompts/
│   ├── biomechanics_coach.txt    # Master persona & kinematic boundary conditions
│   ├── tactical_feedback.txt     # Sub-second live voice instruction synthesis prompt
│   └── session_summary.txt       # Post-training comprehensive scouting report generator
├── schemas/
│   ├── telemetry_packet.json     # Fused IMU + 33-landmark kinematic joint payload
│   ├── deviation_alert.json      # Structured haptic and vocal warning contract
│   └── session_report.json       # Scouting report and drill prescription schema
└── README.md
```

---

## 2. Scoring Formula & Heuristics

The overall technique score $S_{\text{overall}}$ is synthesized as a weighted linear combination of four kinetic chain indicators:

$$S_{\text{overall}} = 0.30 \cdot S_{\text{posture}} + 0.25 \cdot S_{\text{alignment}} + 0.25 \cdot S_{\text{stability}} + 0.20 \cdot S_{\text{efficiency}}$$

### Evaluation Components:
1. **$S_{\text{posture}}$ (Posture Integrity)**:
   - Evaluates elbow extension through release and spinal neutral alignment.
   - Penalizes illegal elbow flexion ($> 15^\circ$ flexion delta) and lumbar hyperextension.
2. **$S_{\text{alignment}}$ (Kinetic Plane Alignment)**:
   - Measures shoulder-to-hip rotational alignment against the intended target vector.
3. **$S_{\text{stability}}$ (Base & Landing Stability)**:
   - Assesses landing knee brace angle ($140^\circ\text{--}165^\circ$) and lateral hip height symmetry ($|Y_{\text{lhip}} - Y_{\text{rhip}}| < 0.04$).
4. **$S_{\text{efficiency}}$ (Power Transfer & Sensor-Fusion Velocity)**:
   - Computes kinematic velocity transfer from run-up deceleration into wrist linear whip velocity ($> 1.8\text{ units/s}$).

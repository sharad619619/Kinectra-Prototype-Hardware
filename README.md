# Kinectra Prototype & Hardware Ecosystem 🏏⚡

> **Real-Time Edge-AI Sports Biomechanics, Wearable IMU Sensor Fusion & Automated Technique Coaching Engine**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32%20%7C%20MPU6050%20%7C%20BNO055-blue.svg)](hardware/)
[![Backend](https://img.shields.io/badge/Backend-FastAPI%20%7C%20WebSockets%20%7C%20OpenCV-green.svg)](backend/)
[![Frontend](https://img.shields.io/badge/Frontend-React%20%7C%20Vite%20%7C%20TailwindCSS-orange.svg)](frontend/)
[![AI-Vision](https://img.shields.io/badge/AI-MediaPipe%20%7C%20Gemini%20Multimodal-purple.svg)](ai/)

---

## 📌 Overview

**Kinectra** is an end-to-end sports biomechanics engineering platform designed for cricket bowling delivery and batting stroke analysis. Combining custom high-frequency **IoT wearable IMU hardware** with **client-side optical computer vision** and **multimodal generative AI coaching**, Kinectra captures high-velocity athletic movements, diagnoses kinetic chain flaws, and delivers instant auditory and visual telemetry.

### Repository Architecture

```text
Kinectra-Prototype-Hardware/
├── README.md               # Master system documentation & integration guide
├── LICENSE                 # Open-source MIT License
├── hardware/               # IoT wearable & edge camera schematics, BOM, firmware
│   ├── components.md       # Full hardware bill of materials (BOM) & specs
│   ├── wiring.md           # Pinouts, circuit diagrams, and electrical schematics
│   └── firmware/           # ESP32 C++ PlatformIO firmware for 200Hz IMU streaming
├── backend/                # Python FastAPI gateway, sensor fusion & video engine
│   ├── main.py             # Dual WebSocket server & REST API
│   ├── requirements.txt    # Python dependencies
│   └── README.md           # Backend service quickstart & API docs
├── frontend/               # React 19 + Vite + Tailwind telemetry dashboard
│   ├── package.json        # Frontend dependencies & build scripts
│   └── README.md           # UI components, state management & setup
├── ai/                     # LLM coaching prompts, schemas & scoring formulas
│   ├── prompts/            # Coach Aryan system prompts & deviation instructions
│   ├── schemas/            # JSON contracts for telemetry, alerts & reports
│   └── README.md           # AI reasoning engine & kinematic scoring model
├── docs/                   # Deep technical specifications & research disclosures
│   ├── architecture.md     # System flowcharts, network topology & sensor fusion
│   └── limitations.md      # Edge constraints, optical occlusion & mitigation
└── demo/                   # Telemetry captures, circuit diagrams & visual proof
    └── screenshots/        # High-resolution architectural & telemetry assets
```

---

*Authored by Sharad Singh ([@sharad619619](https://github.com/sharad619619))*

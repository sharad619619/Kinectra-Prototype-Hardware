import React, { useState, useEffect } from "react";
import { HardwareStatus } from "./components/HardwareStatus";
import { TelemetryGauges } from "./components/TelemetryGauges";
import { PoseViewer } from "./components/PoseViewer";
import { Activity, Play, Square, Volume2, Shield, Flame, Download, RefreshCw } from "lucide-react";

export default function App() {
  const [isRunning, setIsRunning] = useState(true);
  const [sport, setSport] = useState<"bowling" | "batting">("bowling");
  
  // Real-time simulated & gateway metrics
  const [metrics, setMetrics] = useState({
    elbowAngle: 168,
    kneeAngle: 152,
    spineTilt: 22,
    shoulderAlignment: 16,
    techniqueScore: 92,
    handSpeed: 1.84,
    phase: "Delivery Release",
    isActionActive: true,
  });

  const [imuData, setImuData] = useState({
    yaw: 14.8,
    pitch: -6.2,
    roll: 3.5,
    total_g: 1.15,
    impact: false,
  });

  // Cycle telemetry for active demo experience
  useEffect(() => {
    if (!isRunning) return;
    const interval = setInterval(() => {
      setMetrics((prev) => {
        const t = Date.now() / 1000;
        const wave = Math.sin(t * 3);
        const isAction = wave > 0.3;
        return {
          elbowAngle: sport === "bowling" ? (isAction ? 172 : 142) : (isAction ? 85 : 55),
          kneeAngle: isAction ? 148 : 126,
          spineTilt: 20 + Math.sin(t * 2) * 5,
          shoulderAlignment: 15 + Math.cos(t * 2) * 4,
          techniqueScore: isAction ? 94 : 84,
          handSpeed: isAction ? 2.15 : 0.22,
          phase: isAction ? (sport === "bowling" ? "Bowling Release Apex" : "Front-Foot Impact Drive") : "Ready Stance",
          isActionActive: isAction,
        };
      });

      setImuData((prev) => {
        const t = Date.now() / 1000;
        const spike = Math.random() > 0.85;
        return {
          yaw: prev.yaw + (Math.random() - 0.5) * 1.5,
          pitch: prev.pitch + (Math.random() - 0.5) * 1.2,
          roll: prev.roll + (Math.random() - 0.5) * 1.0,
          total_g: spike ? 7.8 : 1.1 + Math.random() * 0.3,
          impact: spike,
        };
      });
    }, 200);

    return () => clearInterval(interval);
  }, [isRunning, sport]);

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 flex flex-col">
      {/* Header */}
      <header className="border-b border-slate-800 bg-slate-900/50 backdrop-blur-md px-6 py-3.5 flex items-center justify-between sticky top-0 z-50">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded-lg bg-orange-500/10 border border-orange-500/30 flex items-center justify-center text-orange-400 font-bold">
            🏏
          </div>
          <div>
            <h1 className="text-base font-bold tracking-tight text-white flex items-center gap-2">
              KINECTRA <span className="text-orange-400 font-mono text-xs uppercase px-1.5 py-0.5 bg-orange-500/10 border border-orange-500/20 rounded">Hardware Lab</span>
            </h1>
            <p className="text-[11px] text-slate-400 font-mono">Edge IoT Sensor Fusion & Kinetic Analysis</p>
          </div>
        </div>

        {/* Sport Selector & Session Controls */}
        <div className="flex items-center gap-3">
          <div className="bg-slate-900 border border-slate-800 p-1 rounded-lg flex text-xs font-semibold">
            <button
              onClick={() => setSport("bowling")}
              className={`px-3 py-1 rounded-md transition-all ${
                sport === "bowling" ? "bg-orange-500 text-white shadow" : "text-slate-400 hover:text-slate-200"
              }`}
            >
              Bowling Delivery
            </button>
            <button
              onClick={() => setSport("batting")}
              className={`px-3 py-1 rounded-md transition-all ${
                sport === "batting" ? "bg-orange-500 text-white shadow" : "text-slate-400 hover:text-slate-200"
              }`}
            >
              Batting Stroke
            </button>
          </div>

          <button
            onClick={() => setIsRunning(!isRunning)}
            className={`px-3 py-1.5 rounded-lg text-xs font-semibold flex items-center gap-1.5 border transition-all ${
              isRunning
                ? "bg-rose-500/10 text-rose-400 border-rose-500/30 hover:bg-rose-500/20"
                : "bg-emerald-500/10 text-emerald-400 border-emerald-500/30 hover:bg-emerald-500/20"
            }`}
          >
            {isRunning ? <Square className="w-3.5 h-3.5" /> : <Play className="w-3.5 h-3.5" />}
            {isRunning ? "Pause Stream" : "Resume Stream"}
          </button>
        </div>
      </header>

      {/* Main Content Layout */}
      <main className="flex-1 max-w-7xl w-full mx-auto p-6 space-y-6">
        {/* Top Metric Gauges */}
        <TelemetryGauges
          elbowAngle={metrics.elbowAngle}
          kneeAngle={metrics.kneeAngle}
          spineTilt={metrics.spineTilt}
          shoulderAlignment={metrics.shoulderAlignment}
          techniqueScore={metrics.techniqueScore}
          sport={sport}
        />

        {/* Central Split: Vision Skeleton + Hardware IMU Telemetry */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          <div className="lg:col-span-2 space-y-3">
            <h2 className="text-xs font-bold uppercase tracking-wider text-slate-400 font-mono">
              Live Optical Computer Vision & Kinetic Reticle
            </h2>
            <PoseViewer
              elbowAngle={metrics.elbowAngle}
              kneeAngle={metrics.kneeAngle}
              spineTilt={metrics.spineTilt}
              actionPhase={metrics.phase}
              isActionActive={metrics.isActionActive}
            />
          </div>

          <div className="space-y-4">
            <h2 className="text-xs font-bold uppercase tracking-wider text-slate-400 font-mono">
              Wearable IoT Node Telemetry
            </h2>
            <HardwareStatus
              isConnected={isRunning}
              batteryPercent={94}
              imuData={imuData}
            />

            {/* AI Coach Aryan Feedback Card */}
            <div className="bg-slate-900/70 border border-slate-800 rounded-xl p-4 shadow-sm">
              <div className="flex items-center gap-2 mb-2">
                <Volume2 className="w-4 h-4 text-orange-400" />
                <h3 className="text-xs font-bold text-slate-200 uppercase font-mono">
                  Coach Aryan Voice Alert
                </h3>
              </div>
              <p className="text-xs text-slate-300 italic bg-slate-950/60 p-3 rounded-lg border border-slate-800/80 leading-relaxed font-sans">
                {metrics.isActionActive
                  ? '"Excellent overhead whip! Front knee brace locked firm at 148°, maximizing ground-reaction momentum."'
                  : '"Maintain preparatory stance. Keep bat face square and center of gravity balanced."'}
              </p>
            </div>
          </div>
        </div>
      </main>
    </div>
  );
}

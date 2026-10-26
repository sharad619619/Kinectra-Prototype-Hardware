import React from "react";
import { Cpu, Wifi, Battery, Zap, Radio, CheckCircle, AlertTriangle } from "lucide-react";

interface HardwareStatusProps {
  isConnected: boolean;
  batteryPercent: number;
  imuData: {
    yaw: number;
    pitch: number;
    roll: number;
    total_g: number;
    impact: boolean;
  };
}

export const HardwareStatus: React.FC<HardwareStatusProps> = ({
  isConnected,
  batteryPercent,
  imuData,
}) => {
  return (
    <div className="bg-slate-900/80 border border-slate-800 rounded-xl p-4 shadow-lg backdrop-blur-md">
      <div className="flex items-center justify-between pb-3 border-b border-slate-800">
        <div className="flex items-center gap-2.5">
          <div className="p-2 bg-blue-500/10 border border-blue-500/20 rounded-lg text-blue-400">
            <Cpu className="w-5 h-5" />
          </div>
          <div>
            <h3 className="font-semibold text-sm text-slate-100 flex items-center gap-2">
              ESP32 Wearable Node
              <span className={`w-2 h-2 rounded-full ${isConnected ? "bg-emerald-400 animate-pulse" : "bg-red-500"}`} />
            </h3>
            <p className="text-[11px] text-slate-400 font-mono">ID: kinectra-node-alpha (200Hz DMP)</p>
          </div>
        </div>

        <div className="flex items-center gap-3 text-xs font-mono">
          <div className="flex items-center gap-1.5 text-slate-300">
            <Battery className="w-4 h-4 text-emerald-400" />
            <span>{batteryPercent}%</span>
          </div>
          <div className="flex items-center gap-1.5 text-slate-300">
            <Wifi className="w-4 h-4 text-blue-400" />
            <span>-54 dBm</span>
          </div>
        </div>
      </div>

      {/* Real-time 6-Axis IMU Stream */}
      <div className="mt-3 grid grid-cols-4 gap-2 text-center font-mono">
        <div className="bg-slate-950/60 border border-slate-800/80 rounded-lg p-2">
          <span className="text-[10px] text-slate-500 uppercase">Roll</span>
          <p className="text-sm font-bold text-sky-400">{imuData.roll.toFixed(1)}°</p>
        </div>
        <div className="bg-slate-950/60 border border-slate-800/80 rounded-lg p-2">
          <span className="text-[10px] text-slate-500 uppercase">Pitch</span>
          <p className="text-sm font-bold text-sky-400">{imuData.pitch.toFixed(1)}°</p>
        </div>
        <div className="bg-slate-950/60 border border-slate-800/80 rounded-lg p-2">
          <span className="text-[10px] text-slate-500 uppercase">Yaw</span>
          <p className="text-sm font-bold text-sky-400">{imuData.yaw.toFixed(1)}°</p>
        </div>
        <div className="bg-slate-950/60 border border-slate-800/80 rounded-lg p-2">
          <span className="text-[10px] text-slate-500 uppercase">Total G</span>
          <p className={`text-sm font-bold ${imuData.impact ? "text-rose-400 animate-bounce" : "text-emerald-400"}`}>
            {imuData.total_g.toFixed(2)}g
          </p>
        </div>
      </div>

      {/* Impact Status Banner */}
      {imuData.impact && (
        <div className="mt-2.5 px-3 py-1.5 bg-rose-500/10 border border-rose-500/30 rounded-lg text-rose-400 text-xs font-semibold flex items-center gap-2 animate-pulse">
          <Zap className="w-4 h-4" />
          High-G Impact Peak Detected (Bat Contact / Delivery Stride)
        </div>
      )}
    </div>
  );
};

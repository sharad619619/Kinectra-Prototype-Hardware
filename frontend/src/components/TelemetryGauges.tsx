import React from "react";
import { Activity, ShieldCheck, AlertCircle } from "lucide-react";

interface TelemetryGaugesProps {
  elbowAngle: number;
  kneeAngle: number;
  spineTilt: number;
  shoulderAlignment: number;
  techniqueScore: number;
  sport: string;
}

export const TelemetryGauges: React.FC<TelemetryGaugesProps> = ({
  elbowAngle,
  kneeAngle,
  spineTilt,
  shoulderAlignment,
  techniqueScore,
  sport,
}) => {
  const isElbowLegal = sport === "bowling" ? elbowAngle >= 150 : elbowAngle >= 65;
  const isKneeBraced = kneeAngle === -1 || kneeAngle >= 135;
  const isSpineNeutral = spineTilt <= 28;

  return (
    <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
      {/* Elbow Gauge */}
      <div className={`p-3.5 rounded-xl border transition-all ${
        isElbowLegal ? "bg-slate-900/60 border-slate-800" : "bg-rose-950/20 border-rose-500/40"
      }`}>
        <div className="flex items-center justify-between text-xs text-slate-400 font-medium mb-1">
          <span>Elbow Angle</span>
          {isElbowLegal ? (
            <ShieldCheck className="w-4 h-4 text-emerald-400" />
          ) : (
            <AlertCircle className="w-4 h-4 text-rose-400" />
          )}
        </div>
        <div className="text-2xl font-bold font-mono text-slate-100">
          {Math.round(elbowAngle)}°
        </div>
        <p className="text-[10px] text-slate-500 mt-1 font-mono">
          {sport === "bowling" ? "Opt: 165°–180° (Legal)" : "Opt: 75°–95° (High Lift)"}
        </p>
      </div>

      {/* Knee Gauge */}
      <div className={`p-3.5 rounded-xl border transition-all ${
        isKneeBraced ? "bg-slate-900/60 border-slate-800" : "bg-amber-950/20 border-amber-500/40"
      }`}>
        <div className="flex items-center justify-between text-xs text-slate-400 font-medium mb-1">
          <span>Front Knee Brace</span>
          {isKneeBraced ? (
            <ShieldCheck className="w-4 h-4 text-emerald-400" />
          ) : (
            <AlertCircle className="w-4 h-4 text-amber-400" />
          )}
        </div>
        <div className="text-2xl font-bold font-mono text-slate-100">
          {kneeAngle === -1 ? "N/A" : `${Math.round(kneeAngle)}°`}
        </div>
        <p className="text-[10px] text-slate-500 mt-1 font-mono">
          Opt: 140°–165° (Braced)
        </p>
      </div>

      {/* Spine Tilt Gauge */}
      <div className={`p-3.5 rounded-xl border transition-all ${
        isSpineNeutral ? "bg-slate-900/60 border-slate-800" : "bg-amber-950/20 border-amber-500/40"
      }`}>
        <div className="flex items-center justify-between text-xs text-slate-400 font-medium mb-1">
          <span>Lateral Spine Tilt</span>
          {isSpineNeutral ? (
            <ShieldCheck className="w-4 h-4 text-emerald-400" />
          ) : (
            <AlertCircle className="w-4 h-4 text-amber-400" />
          )}
        </div>
        <div className="text-2xl font-bold font-mono text-slate-100">
          {Math.round(spineTilt)}°
        </div>
        <p className="text-[10px] text-slate-500 mt-1 font-mono">
          Opt: 15°–28° (Neutral)
        </p>
      </div>

      {/* Technique Score Gauge */}
      <div className="p-3.5 rounded-xl border bg-slate-900/60 border-slate-800">
        <div className="flex items-center justify-between text-xs text-slate-400 font-medium mb-1">
          <span>Technique Index</span>
          <Activity className="w-4 h-4 text-sky-400" />
        </div>
        <div className="text-2xl font-bold font-mono text-sky-400">
          {techniqueScore}<span className="text-xs text-slate-500">/100</span>
        </div>
        <div className="w-full bg-slate-800 rounded-full h-1.5 mt-2">
          <div
            className="bg-sky-500 h-1.5 rounded-full transition-all duration-300"
            style={{ width: `${Math.min(100, Math.max(0, techniqueScore))}%` }}
          />
        </div>
      </div>
    </div>
  );
};

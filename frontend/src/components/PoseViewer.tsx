import React, { useRef, useEffect } from "react";
import { Camera, Zap } from "lucide-react";

interface PoseViewerProps {
  elbowAngle: number;
  kneeAngle: number;
  spineTilt: number;
  actionPhase: string;
  isActionActive: boolean;
}

export const PoseViewer: React.FC<PoseViewerProps> = ({
  elbowAngle,
  kneeAngle,
  spineTilt,
  actionPhase,
  isActionActive,
}) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    let animId: number;
    let t = 0;

    const render = () => {
      t += 0.03;
      const w = canvas.width;
      const h = canvas.height;

      // Dark futuristic canvas background
      ctx.fillStyle = "#030712";
      ctx.fillRect(0, 0, w, h);

      // Telemetry grid overlay
      ctx.strokeStyle = "rgba(30, 41, 59, 0.4)";
      ctx.lineWidth = 1;
      for (let x = 40; x < w; x += 40) {
        ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
      }
      for (let y = 40; y < h; y += 40) {
        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
      }

      // Kinematic stick skeleton mannequin
      const cx = w / 2;
      const headY = 90;
      const neckY = 130;
      const spineLen = 100;
      const spineRad = (spineTilt * Math.PI) / 180;
      const hipX = cx - Math.sin(spineRad) * spineLen;
      const hipY = neckY + Math.cos(spineRad) * spineLen;

      // Shoulders
      const lShX = cx - 35, lShY = neckY;
      const rShX = cx + 35, rShY = neckY;

      // Bowling Arm kinematics
      const armRad = ((180 - elbowAngle) * Math.PI) / 180;
      const rElbX = rShX + 35 * Math.cos(t * 2);
      const rElbY = rShY + 35 * Math.sin(t * 2);
      const rWriX = rElbX + 35 * Math.cos(t * 2 + armRad);
      const rWriY = rElbY + 35 * Math.sin(t * 2 + armRad);

      // Draw Head
      ctx.strokeStyle = "#38bdf8";
      ctx.lineWidth = 3;
      ctx.beginPath();
      ctx.arc(cx, headY, 18, 0, Math.PI * 2);
      ctx.stroke();

      // Draw Bones
      ctx.strokeStyle = "#0ea5e9";
      ctx.lineWidth = 4;
      ctx.lineCap = "round";

      // Spine
      ctx.beginPath(); ctx.moveTo(cx, neckY); ctx.lineTo(hipX, hipY); ctx.stroke();
      // Shoulders
      ctx.beginPath(); ctx.moveTo(lShX, lShY); ctx.lineTo(rShX, rShY); ctx.stroke();
      // Arm
      ctx.strokeStyle = isActionActive ? "#22c55e" : "#f97316";
      ctx.beginPath(); ctx.moveTo(rShX, rShY); ctx.lineTo(rElbX, rElbY); ctx.stroke();
      ctx.beginPath(); ctx.moveTo(rElbX, rElbY); ctx.lineTo(rWriX, rWriY); ctx.stroke();

      // Joint Nodes
      ctx.fillStyle = "#38bdf8";
      [
        { x: cx, y: neckY },
        { x: hipX, y: hipY },
        { x: rShX, y: rShY },
        { x: rElbX, y: rElbY },
        { x: rWriX, y: rWriY },
      ].forEach((p) => {
        ctx.beginPath();
        ctx.arc(p.x, p.y, 4.5, 0, Math.PI * 2);
        ctx.fill();
      });

      animId = requestAnimationFrame(render);
    };

    render();
    return () => cancelAnimationFrame(animId);
  }, [elbowAngle, kneeAngle, spineTilt, isActionActive]);

  return (
    <div className="relative bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-xl aspect-video flex items-center justify-center">
      <canvas ref={canvasRef} width={640} height={360} className="w-full h-full object-contain" />

      {/* Top HUD Reticle */}
      <div className="absolute top-3 left-3 flex items-center gap-2 px-2.5 py-1 bg-black/60 border border-slate-700/60 rounded-md backdrop-blur-sm text-xs font-mono">
        <Camera className="w-3.5 h-3.5 text-sky-400" />
        <span className="text-slate-300">VISION NODE: ACTIVE</span>
      </div>

      <div className="absolute top-3 right-3 flex items-center gap-2 px-2.5 py-1 bg-black/60 border border-slate-700/60 rounded-md backdrop-blur-sm text-xs font-mono">
        <span className={`w-2 h-2 rounded-full ${isActionActive ? "bg-emerald-400 animate-ping" : "bg-slate-500"}`} />
        <span className={isActionActive ? "text-emerald-400 font-bold" : "text-slate-400"}>
          {actionPhase}
        </span>
      </div>

      {/* Bottom Telemetry Stamp */}
      <div className="absolute inset-x-0 bottom-0 bg-slate-950/80 border-t border-slate-800 px-4 py-2 flex items-center justify-between text-[11px] font-mono text-slate-400">
        <div className="flex items-center gap-2">
          <span className="text-orange-400 font-bold">KINECTRA LABS</span>
          <span>• 3D Skeletal Vector Mapping</span>
        </div>
        <div className="text-slate-300">
          Elbow: <span className="text-sky-400 font-semibold">{Math.round(elbowAngle)}°</span> | Spine: <span className="text-sky-400 font-semibold">{Math.round(spineTilt)}°</span>
        </div>
      </div>
    </div>
  );
};

// Rear-wheel-drive bicycle model with load transfer, friction circle, and a Pacejka-style tyre curve.
// Local frame: u = forward speed, v = lateral speed (left +), r = yaw rate (left +).
import { clamp, lerp } from './road.js';

const G = 9.81;

export class Car {
  constructor() {
    // chassis
    this.m = 1320; this.a = 1.22; this.b = 1.38; this.L = this.a + this.b; this.h = 0.48; this.Iz = 2150;
    this.track = 1.58; this.wheelR = 0.33;
    // drivetrain
    this.ratios = [3.3, 2.15, 1.55, 1.2, 0.97, 0.8];
    this.final = 3.7;
    this.idle = 950; this.redline = 7200;
    this.Cd = 0.47; this.Crr = 190;
    // tyres
    this.B = 12; this.C = 1.34;
    this.reset(0, 0, 0, 0);
  }
  reset(x, z, yaw, speed = 0) {
    this.x = x; this.z = z; this.yaw = yaw;
    this.u = speed; this.v = 0; this.r = 0;
    this.gear = speed > 20 ? 3 : speed > 10 ? 2 : 1; this.rpm = this.idle; this.shiftT = 0;
    this.ax = 0; this.ay = 0; this.axF = 0;
    this.steerAngle = 0; this.wheelRot = 0;
    this.slipF = 0; this.slipR = 0; this.spin = 0; this.lock = 0;
    this.throttle = 0; this.brake = 0;
  }
  torque(rpm) {
    // smooth curve peaking ~4600rpm, ~395Nm
    const x = (rpm - 4600) / 3600;
    return 395 * (1 - 0.55 * x * x) * (rpm > this.redline ? 0 : 1);
  }
  maxSteer(u) { return 0.6 / (1 + Math.abs(u) / 24); }
  // steering angle that roughly reaches the grip limit at this speed
  gripSteer(u, mu = 1.05) { const ue = Math.max(Math.abs(u), 1); return (mu * G * this.L) / (ue * ue) + 0.012; }

  // surface: { mu, roll } ; input: { steer, throttle, brake, hand }, assist bool
  step(dt, input, surface, assist) {
    const m = this.m, a = this.a, b = this.b, L = this.L;
    let u = this.u, v = this.v, r = this.r;
    const absU = Math.abs(u);
    const ue = Math.max(absU, 3.5);

    // --- steering ---
    let delta = input.steer * this.maxSteer(u);
    if (assist && absU > 6) {
      // light counter-steer help: align front wheels with the direction of travel when sliding
      const beta = Math.atan2(v, ue);
      delta += clamp(beta * 0.55, -0.18, 0.18);
    }
    this.steerAngle = lerp(this.steerAngle, delta, 1 - Math.exp(-dt * 30));
    delta = this.steerAngle;

    // --- loads (longitudinal transfer, filtered) ---
    const Fz = m * G;
    let Fzf = Fz * b / L - m * this.axF * this.h / L;
    let Fzr = Fz * a / L + m * this.axF * this.h / L;
    Fzf = Math.max(Fzf, Fz * 0.15); Fzr = Math.max(Fzr, Fz * 0.15);
    const mu = surface.mu;
    let capF = mu * Fzf, capR = mu * Fzr * (input.hand ? 0.42 : 1);

    // --- drivetrain ---
    const wheelRpm = absU / this.wheelR * 60 / (2 * Math.PI);
    let ratio = this.ratios[this.gear - 1] * this.final;
    // auto gearbox
    if (this.shiftT <= 0) {
      const rpmNow = wheelRpm * ratio;
      const upAt = 3300 + 3500 * Math.min(1, input.throttle * 1.15);
      if (rpmNow > upAt && this.gear < this.ratios.length && u > 0) { this.gear++; this.shiftT = 0.18; }
      else if (this.gear > 1 && rpmNow < (input.throttle > 0.6 ? 3800 : 1900) && upAt > rpmNow * 1.35) { this.gear--; this.shiftT = 0.12; }
    } else this.shiftT -= dt;
    ratio = this.ratios[this.gear - 1] * this.final;
    const targetRpm = Math.max(this.idle, wheelRpm * ratio + this.spin * 1800);
    this.rpm = lerp(this.rpm, Math.min(targetRpm, this.redline + 150), 1 - Math.exp(-dt * 18));

    let thr = this.shiftT > 0 ? input.throttle * 0.15 : input.throttle;
    let Fdrive = thr * this.torque(this.rpm) * ratio / this.wheelR * 0.9;

    // reverse when nearly stopped and brake held
    let reversing = false;
    if (input.brake > 0.1 && u < 0.8 && input.throttle < 0.05) {
      reversing = true;
      Fdrive = -input.brake * 3200;
    }

    // brakes
    let brakeF = reversing ? 0 : input.brake * 1.15 * Fz;
    const sgnU = u >= 0 ? 1 : -1;
    let Fbf = brakeF * 0.64, Fbr = brakeF * 0.36 + (input.hand ? 0.35 * Fz : 0);
    if (absU < 0.3) { Fbf *= absU / 0.3; Fbr *= absU / 0.3; }

    const alphaR0 = Math.atan2(v - b * r, ue);
    // traction / lockup
    let FxR = Fdrive - Fbr * sgnU;
    let FxF = -Fbf * sgnU;
    this.spin = 0; this.lock = 0;
    if (assist) {
      // traction control + ABS keep forces at ~92% of capacity
      // traction control also trims drive when the rear is already working hard laterally
      const tcLim = capR * clamp(0.92 - Math.abs(alphaR0) * 5, 0.25, 0.92);
      if (Math.abs(FxR) > tcLim) FxR = Math.sign(FxR) * tcLim;
      if (Math.abs(FxF) > capF * 0.95) FxF = Math.sign(FxF) * capF * 0.95;
    } else {
      if (Math.abs(FxR) > capR) { this.spin = clamp((Math.abs(FxR) - capR) / capR, 0, 1); FxR = Math.sign(FxR) * capR * 0.82; }
      if (Math.abs(FxF) > capF) { this.lock = 1; FxF = Math.sign(FxF) * capF * 0.8; }
    }
    if (assist && Fdrive > capR * 0.92) this.spin = 0.15;
    // friction ellipse: remaining lateral capacity
    const latCapR = capR * Math.sqrt(Math.max(0.04, 1 - 0.9 * (FxR / capR) ** 2)) * (this.spin > 0 ? 0.6 : 1);
    const latCapF = capF * Math.sqrt(Math.max(0.04, 1 - 0.9 * (FxF / capF) ** 2)) * (this.lock ? 0.35 : 1);

    // --- lateral tyre forces ---
    const alphaF = Math.atan2(v + a * r, ue) - delta * sgnU;
    const alphaR = Math.atan2(v - b * r, ue);
    const pac = (al) => Math.sin(this.C * Math.atan(this.B * al));
    const FyF = -latCapF * pac(alphaF);
    const FyR = -latCapR * pac(alphaR);
    this.slipF = alphaF; this.slipR = alphaR;

    // --- resistances ---
    const drag = this.Cd * u * absU;
    const roll = (this.Crr + surface.roll * Fz) * clamp(u / 0.5, -1, 1);

    const cosd = Math.cos(delta), sind = Math.sin(delta);
    const Fx = FxR + FxF * cosd - FyF * sind - drag - roll + (surface.slopeG || 0) * m;
    const Fy = FyR + FyF * cosd + FxF * sind + surface.bankG * m;
    let Mz = a * (FyF * cosd + FxF * sind) - b * FyR;
    if (assist && absU > 8) {
      // gentle stability control: damp yaw beyond what the tyres can sustain
      const rMax = (mu * G * 1.05) / ue;
      const ex = Math.abs(r) - rMax;
      if (ex > 0) Mz -= Math.sign(r) * ex * this.Iz * 3.0;
    }

    const du = Fx / m + v * r;
    const dv = Fy / m - u * r;
    const dr = Mz / this.Iz;

    u += du * dt; v += dv * dt; r += dr * dt;

    // low-speed kinematic blend (tyre model is singular near standstill)
    if (absU < 4) {
      const w = 1 - absU / 4;
      r = lerp(r, u * Math.tan(delta) / L, clamp(w * dt * 14, 0, 1));
      v = lerp(v, 0, clamp(w * dt * 10, 0, 1));
    }
    if (!reversing && absU < 0.25 && input.throttle < 0.02) { u *= 0.9; }

    this.ax = du - v * r; this.ay = dv + u * r;
    this.axF = lerp(this.axF, clamp(this.ax, -12, 10), 1 - Math.exp(-dt * 7));
    this.u = u; this.v = v; this.r = r;

    // integrate pose
    this.yaw += r * dt;
    const sy = Math.sin(this.yaw), cy = Math.cos(this.yaw);
    // forward = (sin, cos), left = (cos, -sin)
    this.x += (sy * u + cy * v) * dt;
    this.z += (cy * u - sy * v) * dt;
    this.wheelRot += (u / this.wheelR) * dt;
    this.throttle = input.throttle; this.brake = input.brake;
  }
  get speed() { return Math.hypot(this.u, this.v); }
}

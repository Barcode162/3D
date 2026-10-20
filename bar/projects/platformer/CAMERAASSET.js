/* =========================================================
   dHP — tuning.js
   ALL camera, input, movement and feel controls live here.
   Edit these values to change the feel. Nothing else needs
   to be touched. Imported by game.js as `TUNE`.
   ========================================================= */

export const TUNE = {

  /* -------------------------------------------------------
     ROOM  — geometry of the play space
     ------------------------------------------------------- */
  ROOM: 10,             // half extent (20 x 20 m floor)
  WALL_H: 4,            // wall height (m)
  WALL_T: 0.2,          // wall thickness (m)
  LINE_W: 0.05,         // grid line width (m)

  /* -------------------------------------------------------
     PLAYER  — physical body
     ------------------------------------------------------- */
  EYE: 1.6,             // camera height above feet (m)
  RADIUS: 0.3,          // collision radius (m)
  WALK_SPEED: 3.0,      // top speed walking (m/s)
  RUN_SPEED: 6.0,       // top speed while RUN held (m/s)
  GRAVITY: -18,         // downward accel (m/s^2)

  /* -------------------------------------------------------
     INPUT  — joystick, look, response
     ------------------------------------------------------- */
  ZONE: 0.06,           // stick dead zone (0..1)
  ACCEL_K: 16,          // velocity chase rate. higher = snappier start/stop
  LOOK_SENS: 0.0032,    // camera look sensitivity (rad per px)
  PITCH_MAX: 1.45,      // up/down look limit (rad)
  STICK_K_DOWN: 22,     // knob follow rate while touching. higher = tighter
  STICK_K_UP: 18,       // knob return rate on release. higher = snappier

  /* -------------------------------------------------------
     FOV  — field of view
     90 (idle/walk) -> 110 (full stick), plus a small bump
     while RUN is held. FOV_K_IN is snappy, FOV_K_OUT is calm.
     ------------------------------------------------------- */
  FOV_BASE: 90,             // resting FOV
  FOV_RUN: 110,             // peak FOV at full stick
  FOV_K_IN: 6.5,            // zoom-in rate (fast = momentum feel)
  FOV_K_OUT: 3.5,           // zoom-out rate (slow = calm return)
  FOV_MAG_START: 0.1,       // stick mag where FOV starts moving (10%)
  FOV_SPRINT_BUMP: 0.075,   // +7.5% of FOV_RUN while RUN held (~8 deg)

  /* -------------------------------------------------------
     TILT  — camera roll into strafe
     Snappy in, slow return to level.
     ------------------------------------------------------- */
  TILT_PER_MPS: 0.018,  // roll (rad) per m/s of lateral speed
  TILT_K_IN: 14,        // tilt-in rate (fast)
  TILT_K_OUT: 4.0,      // return-to-level rate (slow)

  /* -------------------------------------------------------
     SHAKE  — running camera jitter
     Only kicks in above SHAKE_DEADZONE of top run speed.
     ------------------------------------------------------- */
  SHAKE_DEADZONE: 0.55, // speedNorm below this = no shake
  SHAKE_AMP_PITCH: 0.012,
  SHAKE_AMP_YAW:   0.014,
  SHAKE_AMP_ROLL:  0.016,

  /* -------------------------------------------------------
     BOB  — walk/run camera bob
     Amplitude scales with speedNorm^BOB_CURVE, so walking is
     muted and sprinting gets the full bounce. BOB_RATE sets
     cadence (frequency scales with speed automatically).
     ------------------------------------------------------- */
  CAM_K: 16,            // camera position smoothing rate
  BOB_AMP: 0.05,        // peak vertical bob (m)
  BOB_SWAY: 0.7,        // lateral sway as fraction of BOB_AMP
  BOB_RATE: 1.4,        // radians of phase per metre travelled
  BOB_K: 9,             // bob amplitude smoothing rate
  BOB_CURVE: 2.0,       // 1 = linear, 2 = walking muted, 3 = very muted
};
// The standard camera spots for "looks the same" checks (VIS in
// docs/archive/RESTRUCTURE.md): CAM / ?cam= strings "x,y,z,yaw,pitch" (degrees).
export const SPOTS = {
  door: '0.4,1.65,5.2,0,0',          // where you spawn
  mirror: '6.0,1.65,3.6,90,0',       // facing the big mirror (right wall)
  bartop: '-2.5,1.65,-2.6,0,-20',    // at the bar, looking at the counter
  backbar: '-1.0,1.65,-3.2,0,8',     // the back bar shelves over the counter
  piano: '-3.6,1.65,3.2,-115,-10',   // the stage with the grand piano
};

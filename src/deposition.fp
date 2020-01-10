#string emscripten

varying vec3   v_pos;

uniform float  u_ZNear;
uniform float  u_ZFar;

uniform float  u_extruder;

varying float  v_dangling;
varying float  v_overlap;

void main()
{  
  float z   = (-v_pos.z - u_ZNear) / (u_ZFar - u_ZNear);
  float z_h = floor(z * 256.0) / 255.0;
  float z_l = fract(z * 256.0);
  float d   = clamp(v_dangling * 0.5, 0.0 ,0.45);
  float o   = clamp(v_overlap  * 0.5, 0.0, 0.5);
  gl_FragColor = vec4(z_l, z_h, (o == 0.0 ? d : 0.5+o), u_extruder);
}

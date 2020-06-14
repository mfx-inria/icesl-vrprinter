#string emscripten

varying vec3   v_pos;

uniform float  u_ZNear;
uniform float  u_ZFar;

uniform float  u_tranparency; // never 0, 1 for extruder 0

uniform int    u_extruder;

uniform float  u_bridge;

varying float  v_dangling;
varying float  v_overlap;

void main()
{  
  float z   = (-v_pos.z - u_ZNear) / (u_ZFar - u_ZNear); // normalize z coord wrt frustum
  float z_h = floor(z * 256.0) / 255.0; // 8 second bits, integer part of z mapped to the 0-255 range (i.e., 1 byte range) then normalized to 0-1 range
  float z_l = fract(z * 256.0); // 8 first bits, fractional part of z mapped to the 0-255 range (i.e., 1 byte range). it's always in the 0-1 range
  float d   = clamp(v_dangling * 0.5, 0.0 ,0.45);
  float o   = clamp(v_overlap  * 0.5, 0.0, 0.45);

  vec3 clr = vec3(z_l, z_h, (o == 0.0 ? d : 0.5 + o));

  //if (u_bridge == 1.0) { // special case for bridges
    //clr = vec3(z_l, z_h, 1.0);
  //}

  if (u_extruder > 0) {
    clr = vec3(z_l, z_h, 1.0);
  }

  gl_FragColor = vec4(clr, u_tranparency);
}

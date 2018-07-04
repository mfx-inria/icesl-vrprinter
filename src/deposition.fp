#string emscripten

varying vec3   v_pos;

uniform float  u_ZNear;
uniform float  u_ZFar;

varying float  v_dangling;

void main()
{  
  float z   = (-v_pos.z - u_ZNear) / (u_ZFar - u_ZNear);
  float z_h = floor(z * 256.0) / 255.0;
  float z_l = fract(z * 256.0);
  gl_FragColor = vec4(z_l, z_h, pow(v_dangling,5.0), 1.0);
}

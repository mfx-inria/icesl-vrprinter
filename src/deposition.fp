#string emscripten

varying vec3   v_pos;

uniform float  u_ZNear;
uniform float  u_ZFar;

void main()
{  
  float z = 1.0 - (v_pos.z + u_ZNear) / (u_ZFar + u_ZNear);
  float z_h = floor(z * 255.0) / 255.0;
  float z_l = fract(z * 255.0);
  gl_FragColor = vec4(z_l, z_h,0.0, 1.0);
}

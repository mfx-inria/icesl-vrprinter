#string emscripten

varying vec2      v_uv;

uniform sampler2D u_texpts;
uniform vec3      u_pixsz;
uniform vec2      u_texscl;

uniform float  u_ZNear;
uniform float  u_ZFar;

uniform int    u_color_overhangs;

float decode_float(vec4 v) { return (v.x*256.0 + v.y*255.0*256.0) / 65536.0; }

void main()
{
  vec4 tex = texture2D(u_texpts, v_uv * u_texscl);

  if (tex.w == 0.0) discard;

  float z_00 = decode_float(tex);
  float z_01 = decode_float(texture2D(u_texpts, v_uv * u_texscl + u_pixsz.xz));
  float z_10 = decode_float(texture2D(u_texpts, v_uv * u_texscl + u_pixsz.zy));

  vec3 p = vec3(0.3 * u_pixsz.xz / u_texscl, z_01 - z_00);
  vec3 q = vec3(0.3 * u_pixsz.zy / u_texscl, z_10 - z_00);

  vec3 nrm = normalize(cross(p, q));

  if (u_color_overhangs == 1) {
    float d = tex.z;
    float o = 0.0;
    if (d >= 0.5) {
      o = (d - 0.5)*2.0; 
      d = 0.0;
    } else {
      d = d * 2.0;
    }
    vec3 clr = vec3(1.0 - o,1.0 - d - o,1.0 - d);
    gl_FragColor = vec4(clr * nrm.zzz, 1.0);
  } else {
    if (tex.w < 0.5) {
      gl_FragColor = vec4(nrm.z * vec3(1.0,1.0,1.0), 1.0);
    } else {
      gl_FragColor = vec4(nrm.z * vec3(0.5, 0.5, 1.0), 1.0);
    }
  }
  // gl_FragColor = tex;
}

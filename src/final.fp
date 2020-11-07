#string settings

in vec2      v_uv;

uniform sampler2D u_texpts;
uniform vec3      u_pixsz; // v(1/w, 1/h, 0) size pixel screen space // texture space (u,v) - screen space
uniform vec2      u_texscl; // remapping from squared texture (g-buffer, deposition.fp) to screen space

uniform float  u_ZNear;
uniform float  u_ZFar;

uniform int    u_color_overhangs;

out vec4 fragColor;

float decode_float(vec4 v) { return (v.x*256.0 + v.y*255.0*256.0) / 65536.0; } // it could be vec2 // reconstruct screen-space z
float decode_tool(vec4 v) { return v.w * 255.0; }

void main()
{
  vec4 tex = texture(u_texpts, v_uv * u_texscl); // current pixel

  if (tex.w == 0.0) discard; // texture was cleared before, then z=0 means we didn't write to it

  float z_00 = decode_float(tex); // current z of texture
  float z_01 = decode_float(texture(u_texpts, v_uv * u_texscl + u_pixsz.xz)); // z pixel to the right (should be called z_10)
  float z_10 = decode_float(texture(u_texpts, v_uv * u_texscl + u_pixsz.zy)); // z pixel below (should be called z_01)

  float tool = decode_tool(tex);

  // loca average (squared window) of depth
  // this could be optimized
  const int N = 11;
  float z_avg = 0.0;
  float num = 0.0;
  for (int j = -N; j <= N; j++) {
    for (int i = -N; i <= N; i++) {
      vec4 zhl = texture(u_texpts, v_uv * u_texscl + u_pixsz.xy * vec2(i, j));
      if (zhl.w > 0.0) {
        float z_ij = decode_float(zhl);
        z_avg = z_avg + z_ij;
        num += 1.0;
      }
    }
  }
  z_avg = z_avg / num;

  vec3 p = vec3(0.3 * u_pixsz.xz / u_texscl, z_01 - z_00); // current - right
  vec3 q = vec3(0.3 * u_pixsz.zy / u_texscl, z_10 - z_00); // current - below

  vec3 nrm = normalize(cross(p, q)); // screen space normal normal
  
  // ambient occlusion (ssao)
  float ao = 1.0;  // ambient occlusion factor
  if (z_avg < z_00) {// z average behind of current z (i.e., concave region) -- darken color
    ao = max(0.0,1.0 - 100.0 * (z_00 - z_avg)); // constants here could be tweaked
    ao = 0.1 + 0.9 * ao; // darken a bit
  } // no else because z in front of current z (i.e., convex region) -- color is kept as is

  if (u_color_overhangs == 1) { // overhang in red
    float d = tex.z;
    float o = 0.0;
    vec3 clr;
    if (d == 1.0) {
      clr = vec3(0.7, 0.7,0.0);
    } else {
      if (d >= 0.5) {
        o = (d - 0.5) * 2.0;
        d = 0.0;
      } else {
        d = d * 2.0;
      }
      clr = vec3(1.0 - o, 1.0 - d - o, 1.0 - d);
    }
    fragColor = vec4(ao * clr * nrm.zzz, 1.0);
  } else {
    vec3 final_clr = vec3(1.0, 1.0, 1.0); // white
    if (tool == 2.0){
      final_clr = vec3(0.271, 0.761, 0.765); // IceSL signature blue
    } else if (tool == 3.0) {
      final_clr = vec3(0.576, 0.584, 0.596); // IceSL brush 0
    } else if (tool == 4.0) {
      final_clr = vec3(0.0, 0.365, 0.365); // IceSL brush 1
    } else if (tool == 5.0) {
      final_clr = vec3(0.694, 0.494, 0.439); // IceSL brush 2
    }

    fragColor = vec4(ao * nrm.z * final_clr, 1.0); // ssao * diffuse * color
  }
  //gl_FragColor = tex;
}

#string emscripten

varying vec4   v_color;

varying vec2      v_tex;
uniform sampler2D u_tex;
uniform int       u_use_tex;

void main()
{
  if (u_use_tex == 1) {
    gl_FragColor = vec4( fract(texture(u_tex, v_tex).x /10.0) );
  } else {
    gl_FragColor = v_color;
  }
}

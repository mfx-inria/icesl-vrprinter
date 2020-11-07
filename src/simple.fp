#string settings

in vec4 v_color;

in vec2 v_tex;
uniform sampler2D u_tex;
uniform int       u_use_tex;

out vec4 fragColor;

void main()
{
  if (u_use_tex == 1) {
    fragColor = vec4( fract(texture(u_tex, v_tex).x /10.0) );
  } else {
    fragColor = v_color;
  }
}

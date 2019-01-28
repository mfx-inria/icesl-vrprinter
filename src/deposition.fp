#string emscripten

varying vec3   v_pos;

uniform float  u_ZNear;
uniform float  u_ZFar;

uniform float  u_extruder;

varying float  v_dangling;

void main()
{  
  float z   = (-v_pos.z - u_ZNear) / (u_ZFar - u_ZNear);
  float z_h = floor(z * 256.0) / 255.0;
  float z_l = fract(z * 256.0);
  gl_FragColor = vec4(z_l, z_h, pow(v_dangling,5.0), u_extruder);

  /*
  if (u_extruder == 0){
	gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0); //magenta
  } else if (u_extruder == 1){
	gl_FragColor = vec4(0.0, 1.0, 1.0, 1.0); //cyan
  } else if (u_extruder == 2){
	gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0); //yellow
  } else if (u_extruder == 3){
	gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0); //black
  } else if (u_extruder == 4){
	gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0); //white
  } else{
	gl_FragColor = vec4(z_l, z_h, pow(v_dangling,5.0), u_extruder);
  }
  */
}

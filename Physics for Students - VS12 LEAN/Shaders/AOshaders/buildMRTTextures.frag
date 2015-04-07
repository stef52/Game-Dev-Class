#version 130
#extension GL_ARB_texture_rectangle : require

in vec4 pos;
out vec4 Pos;
out vec4 Norm;

void main() {    
  Pos = pos / pos.w;
  vec3 n = -cross(dFdx(Pos.xyz), dFdy(Pos.xyz)); 
  if (n.z < 0.0) n = -n; //WILF BUG FIX: On my ATI card, one of the differentials has the wrong sign (this fixes it).
  Norm = vec4(normalize(n), 1.0);
}
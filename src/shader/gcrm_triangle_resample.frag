//#extension GL_EXT_gpu_shader4 : enable
#version 430 core
uniform sampler1D tf;

in G2F{
	float scalar;
}g2f;

#define invPi 0.3183
#define inv2Pi 0.1591
#define alphaThreshold 0.1

vec4 GetColor(in float scale){
	vec4   tempColor = texture(tf, scale);
	return tempColor;
}

/*
//debugging resampling by visualizing resampled mesh
out vec4 out_Color;
void main(){
	vec4 color = GetColor(g2f.scalar);
	out_Color = color;
}
*/

out float out_Color;
void main(){
	out_Color = g2f.scalar;
}
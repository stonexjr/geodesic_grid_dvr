//#extension GL_EXT_gpu_shader4 : enable
#version 430 core
uniform sampler2D uWorldColorMap;
uniform sampler2D uWorldAlphaMap;
uniform sampler1D tf;

in G2F{
	vec3 normal;
	vec3 o_pos;
	vec4 e_pos;
	float scalar;
}g2f;

uniform vec4 e_lightPos;
uniform bool enableLight;
uniform int drawLine;
uniform int drawWorldMap;

out vec4 out_Color;

#define invPi 0.3183
#define inv2Pi 0.1591
#define alphaThreshold 0.1

vec4 GetColor(in float scale){
	vec4 tempColor = texture(tf, scale);
	return tempColor;
}

void main(){
	float intensity=1.0;
	vec4 color ;
	vec3 vNormal = normalize(g2f.normal);
	if (vNormal.z < -0.01){
		//discard;//only draw visible surface.
	}
	color = GetColor(g2f.scalar);
	if (enableLight){
		vec3 L = normalize((e_lightPos - g2f.e_pos).xyz);
		intensity = max(dot(L, vNormal), 0);//dot(normalize(lightDir), -vNormal));
	}
	out_Color = color * intensity;
}
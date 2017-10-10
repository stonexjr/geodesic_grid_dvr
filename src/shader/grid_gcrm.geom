#version 430 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in V2G{
	float scalar;
}v2g[];

out G2F{
	vec3 normal;
	vec3 o_pos;
	vec4 e_pos;
	float scalar;
}g2f;


uniform mat4 modelViewMat;
uniform mat4 projMat;
uniform mat3 normalMat;

uniform int drawLine;
uniform int drawWorldMap;
uniform float ratio;
uniform int maxInvisibleCellId;

#define  GLOBLE_RADIUS 1.0

void main(void)
{
	for (int i = 0; i < gl_in.length(); i++)
	{
		vec4 latlonCellid = gl_in[i].gl_Position;//lat,lon,cellid,0
		vec4 xyzw = vec4(GLOBLE_RADIUS * cos(latlonCellid.x) * sin(latlonCellid.y),
						 GLOBLE_RADIUS * sin(latlonCellid.x),
						 GLOBLE_RADIUS * cos(latlonCellid.x) * cos(latlonCellid.y),
						 1.0);

		gl_Position = projMat * modelViewMat * xyzw;
		g2f.normal = normalMat * normalize(xyzw.xyz);
		g2f.o_pos = xyzw.xyz;
		g2f.scalar = v2g[i].scalar;
		g2f.e_pos = modelViewMat * xyzw;
		EmitVertex();
	}
	EndPrimitive();
}
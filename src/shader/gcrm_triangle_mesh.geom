#version 430 core

layout (points) in;
layout (triangle_strip, max_vertices = 3) out;
//layout (points, max_vertices = 1) out;

in V2G{
	flat int corner_id;
}v2g[];

out G2F{
	vec3 normal;
	vec3 o_pos;
	vec4 e_pos;
	float scalar;
}g2f;

//3D transformation matrices
uniform mat4 modelViewMat;
uniform mat4 projMat;
uniform mat3 normalMat;
//connectivity
uniform samplerBuffer  center_lat;
uniform samplerBuffer  center_lon;
uniform samplerBuffer  center_val;
uniform isamplerBuffer corner_cells;
uniform int nLayers;
uniform int iLayers;
//
uniform float timing;
uniform int drawLine;
uniform int drawWorldMap;
uniform float ratio;
uniform int max_invisible_cell_id;

#define  PI_OVER_2 1.5707963
#define  _2_OVER_PI 0.63661978
#define  PI 3.1415926
#define  INV_PI 0.31830989
#define  GLOBLE_RADIUS PI_OVER_2

vec3 sphere_plane_transition(vec3 xyz, float lat, float lon)
{
	return mix(xyz, vec3(lon, lat, 0.0), timing);
}


void main(void)
{
	if (v2g[0].corner_id < max_invisible_cell_id)
		return;

	int iTime = 0;
	//only consider outter most surface mesh.
	int cornerId = v2g[0].corner_id;
	//query three neighboring cell id.
	ivec3 cellId3 = texelFetch(corner_cells, cornerId).xyz;
	//query the lat lon of the three neighboring cells.

	int cellIds[3];
	cellIds[0] = cellId3.x;
	cellIds[1] = cellId3.y;
	cellIds[2] = cellId3.z;

	for (int i = 0; i < 3;i++)
	{
		int   cell_id = cellIds[i];
		float lat = texelFetch(center_lat, cell_id).r;
		float lon = texelFetch(center_lon, cell_id).r;

		vec4 xyzw = vec4(
			GLOBLE_RADIUS * cos(lat) * sin(lon),
			GLOBLE_RADIUS * sin(lat),
			GLOBLE_RADIUS * cos(lat) * cos(lon),
			1.0);

		xyzw.xyz = sphere_plane_transition(xyzw.xyz, lat, lon);

		gl_Position = projMat * modelViewMat * xyzw;
		g2f.normal = normalMat * normalize(xyzw.xyz);
		g2f.o_pos = xyzw.xyz;
		g2f.scalar = texelFetch(center_val, cell_id*nLayers + iLayers).r;//first time slice
		g2f.e_pos = modelViewMat * xyzw;
		EmitVertex();
	}
	EndPrimitive();
}
/*
*/
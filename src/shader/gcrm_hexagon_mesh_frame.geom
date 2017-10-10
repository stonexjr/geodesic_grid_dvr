#version 430 core

layout (points) in;
layout (line_strip, max_vertices = 6) out;

in V2G{
	flat int center_id;
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
uniform samplerBuffer  corner_lat;
uniform samplerBuffer  corner_lon;
uniform samplerBuffer  center_val;
uniform isamplerBuffer cell_corners;
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
	if (v2g[0].center_id < max_invisible_cell_id)
		return;

	int iTime = 0;//time slice in dataset
	//only consider outter most surface mesh.
	int centerId = v2g[0].center_id;
	//query six neighboring corner id's.
	//query the lat lon of the three neighboring cells.
	int cornerIds6[6];
	for (int i = 0; i < 6; i++)
	{
		cornerIds6[i] = texelFetch(cell_corners, centerId * 6 + i).x;
	}
	//generate line strip 
	//first corners
	float center_scalar = texelFetch(center_val, centerId*nLayers + iLayers).r;//first time slice

	for (int i = 0; i < 6; i++)
	{
		int   corner_id = cornerIds6[i];
		float lat = texelFetch(corner_lat, corner_id).r;
		float lon = texelFetch(corner_lon, corner_id).r;

		vec4 xyzw = vec4(
			GLOBLE_RADIUS * cos(lat) * sin(lon),
			GLOBLE_RADIUS * sin(lat),
			GLOBLE_RADIUS * cos(lat) * cos(lon),
			1.0);

		xyzw.xyz = sphere_plane_transition(xyzw.xyz, lat, lon);

		gl_Position = projMat * modelViewMat * xyzw;
		g2f.normal = normalMat * normalize(xyzw.xyz);
		g2f.o_pos  = xyzw.xyz;
		g2f.scalar = center_scalar;
		g2f.e_pos  = modelViewMat * xyzw;
		EmitVertex();
	}
	EndPrimitive();
}
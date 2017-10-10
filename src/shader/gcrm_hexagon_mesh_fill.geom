#version 430 core

layout (points) in;
layout (triangle_strip, max_vertices = 64) out;

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

	int iTime = 0;
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
	//first corner
	float center_scalar = texelFetch(center_val, centerId*nLayers + iLayers).r;//first time slice
	int   corner_id = cornerIds6[0];
	float lat = texelFetch(corner_lat, corner_id).r;
	float lon = texelFetch(corner_lon, corner_id).r;
	vec4 xyzw0 = vec4(
		GLOBLE_RADIUS * cos(lat) * sin(lon),
		GLOBLE_RADIUS * sin(lat),
		GLOBLE_RADIUS * cos(lat) * cos(lon),
		1.0);
	xyzw0.xyz = sphere_plane_transition(xyzw0.xyz, lat, lon);

	//second corner
	corner_id = cornerIds6[1];
	lat = texelFetch(corner_lat, corner_id).r;
	lon = texelFetch(corner_lon, corner_id).r;
	vec4 xyzw_prev = vec4(
		GLOBLE_RADIUS * cos(lat) * sin(lon),
		GLOBLE_RADIUS * sin(lat),
		GLOBLE_RADIUS * cos(lat) * cos(lon),
		1.0);
	xyzw_prev.xyz = sphere_plane_transition(xyzw_prev.xyz, lat, lon);

	for (int i = 2; i < 6; i++)
	{//create triangle fans that form current hexagon cell.
		corner_id = cornerIds6[i];
		lat = texelFetch(corner_lat, corner_id).r;
		lon = texelFetch(corner_lon, corner_id).r;

		vec4 xyzw_new = vec4(
			GLOBLE_RADIUS * cos(lat) * sin(lon),
			GLOBLE_RADIUS * sin(lat),
			GLOBLE_RADIUS * cos(lat) * cos(lon),
			1.0);
		

		gl_Position = projMat * modelViewMat * xyzw0;
		g2f.normal = normalMat * normalize(xyzw0.xyz);
		g2f.o_pos  = xyzw0.xyz;
		g2f.scalar = center_scalar;
		g2f.e_pos  = modelViewMat * xyzw0;
		EmitVertex();

		gl_Position = projMat * modelViewMat * xyzw_prev;
		g2f.normal = normalMat * normalize(xyzw_prev.xyz);
		g2f.o_pos  = xyzw_prev.xyz;
		g2f.scalar = center_scalar;
		g2f.e_pos  = modelViewMat * xyzw_prev;
		EmitVertex();

		xyzw_new.xyz = sphere_plane_transition(xyzw_new.xyz, lat, lon);
		gl_Position = projMat * modelViewMat * xyzw_new;
		g2f.normal = normalMat * normalize(xyzw_new.xyz);
		g2f.o_pos  = xyzw_new.xyz;
		g2f.scalar = center_scalar;
		g2f.e_pos  = modelViewMat * xyzw_new;
		EmitVertex();

		EndPrimitive();
		xyzw_prev = xyzw_new;
	}
}
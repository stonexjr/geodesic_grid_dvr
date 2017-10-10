#version 430 core

layout (points) in;
layout (triangle_strip, max_vertices = 3) out;
//layout (points, max_vertices = 1) out;

in V2G{
	flat int corner_id;
}v2g[];

out G2F{
	float scalar;
}g2f;

//3D transformation matrices
uniform mat4 modelViewMat;
uniform mat4 projMat;
//connectivity
uniform samplerBuffer  center_lat;
uniform samplerBuffer  center_lon;
uniform samplerBuffer  center_val;
uniform isamplerBuffer corner_cells;
uniform int nLayers;
uniform int iLayers;
uniform bool remove_wrapover;
//
#define   PI_OVER_2 1.5707963
#define  _2_OVER_PI 0.63661978
#define     PI 3.1415926
#define  _2xPI 6.2831852
#define  INV_PI 0.31830989
#define  GLOBLE_RADIUS PI_OVER_2


void main(void)
{
	//only consider outter most surface mesh.
	int cornerId = v2g[0].corner_id;
	//query three neighboring cell id.
	ivec3 cellId3 = texelFetch(corner_cells, cornerId).xyz;
	//query the lat lon of the three neighboring cells.

	int cellIds[3];
	cellIds[0] = cellId3.x;
	cellIds[1] = cellId3.y;
	cellIds[2] = cellId3.z;
	vec2 lonlats[3];//lat,lon of three vertices of current triangle.

	for (int i = 0; i < 3;i++)
	{
		int   cell_id = cellIds[i];
		float lat = texelFetch(center_lat, cell_id).r;
		float lon = texelFetch(center_lon, cell_id).r;
		lonlats[i] = vec2(lon, lat);
	}
	//check wrap-over artifacts.
	if (remove_wrapover)
	{
		if (abs(lonlats[0].x - lonlats[1].x) > PI){
			if (lonlats[0].x > 0)
			{
				lonlats[0].x -= _2xPI;//move corner[0] that is to the right boundary of screen all the way to the left
			}
			if (lonlats[1].x > 0)
			{
				lonlats[1].x -= _2xPI;
			}
		}
		if (abs(lonlats[1].x - lonlats[2].x) > PI){
			if (lonlats[1].x > 0)
			{
				lonlats[1].x -= _2xPI;
			}
			if (lonlats[2].x > 0)
			{
				lonlats[2].x -= _2xPI;
			}
		}
		if (abs(lonlats[0].x - lonlats[2].x) > PI)
		{
			if (lonlats[0].x > 0)
			{
				lonlats[0].x -= _2xPI;
			}
			if (lonlats[2].x > 0)
			{
				lonlats[2].x -= _2xPI;
			}
		}
	}

	for (int i = 0; i < 3; i++)
	{
		int   cell_id = cellIds[i];
		vec4 xyzw = vec4(lonlats[i], 0.0, 1.0);

		gl_Position = projMat * modelViewMat * xyzw;
		g2f.scalar = texelFetch(center_val, cell_id*nLayers + iLayers).r;//first time slice
		EmitVertex();
	}
	EndPrimitive();
}
/*
*/
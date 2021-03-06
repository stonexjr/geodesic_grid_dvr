/*
Copyright (c) 2013-2017 Jinrong Xie (jrxie at ucdavis dot edu)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.
*/

#version 430 core

layout (points) in;
layout (triangle_strip, max_vertices = 3) out;

in V2G{
	flat int corner_id;
}v2g[];

out G2F{
	flat int triangle_id;
	smooth vec3  o_pos;
}g2f;

//3D transformation matrices
uniform mat4 modelViewMat;
uniform mat4 projMat;
//connectivity
uniform samplerBuffer  center_lat;
uniform samplerBuffer  center_lon;
uniform isamplerBuffer corner_cells;

#define  PI_OVER_2 1.5707963
#define  GLOBLE_RADIUS PI_OVER_2

void main(void)
{
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

		gl_Position = projMat * modelViewMat * xyzw;
		g2f.o_pos = xyzw.xyz;
		g2f.triangle_id = cornerId;
		EmitVertex();
	}
	EndPrimitive();
}
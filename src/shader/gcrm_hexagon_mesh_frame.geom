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

#define  PI_OVER_2 1.5707963
#define  GLOBLE_RADIUS PI_OVER_2

void main(void)
{
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

		gl_Position = projMat * modelViewMat * xyzw;
		g2f.normal = normalMat * normalize(xyzw.xyz);
		g2f.o_pos  = xyzw.xyz;
		g2f.scalar = center_scalar;
		g2f.e_pos  = modelViewMat * xyzw;
		EmitVertex();
	}
	EndPrimitive();
}
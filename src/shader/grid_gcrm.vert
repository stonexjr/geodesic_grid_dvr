#version 430 core

in vec3 in_latLonCellId;
in float in_Scalar;

out V2G{
	float scalar;
}v2g;

void main()
{
	
	v2g.scalar = in_Scalar;
	gl_Position = vec4(in_latLonCellId, 0.0);
	/*
	float lat = in_latLonCellId.x;
	float lon = in_latLonCellId.y;
	vec4 glVertex = vec4(GLOBLE_RADIUS*cos(,1.0);//gl_Vertex;

	//if ((in_CellId-1 > 100))
	if (drawWorldMap==0 && (in_CellId-1 < maxInvisibleCellId))
	{
		glVertex.xyz *= 0;//set to 0 to make it "invisible"
	}
	if (drawLine != 0){
		//dilate the sphere mesh a little, so that the line is clearly visible
		glVertex.xyz = glVertex.xyz * 1.01;
	}
	if (drawWorldMap != 0){
		glVertex.xyz = glVertex.xyz * ratio;
	}
	gl_Position = projMat * modelViewMat * glVertex;//gl_ModelViewProjectionMatrix * glVertex;

	frag.normal = normalMat * normalize(in_Position);//gl_NormalMatrix * gl_Normal;    
	frag.o_pos  = glVertex.xyz;
	frag.scalar = in_Scalar;
	frag.e_pos  = modelViewMat * vec4(frag.o_pos, 1.0);
	*/
}
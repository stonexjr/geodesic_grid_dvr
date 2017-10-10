#version 430 core

in int in_corner_id;

out V2G{
	flat int corner_id;
}v2g;

void main()
{
	v2g.corner_id = in_corner_id;
}
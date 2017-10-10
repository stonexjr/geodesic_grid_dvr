#version 430 core

in int in_center_id;

out V2G{
	flat int center_id;
}v2g;

void main()
{
	v2g.center_id = in_center_id;
}
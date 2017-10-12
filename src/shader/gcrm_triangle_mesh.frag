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

//#extension GL_EXT_gpu_shader4 : enable
#version 430 core
uniform sampler1D tf;

in G2F{
    vec3 normal;
    vec3 o_pos;
    vec4 e_pos;
    float scalar;
}g2f;

uniform vec4 e_lightPos;
uniform bool enableLight;

out vec4 out_Color;


vec4 GetColor(in float scale){
    vec4 tempColor = texture(tf, scale);
    return tempColor;
}

void main(){
    float intensity=1.0;
    vec4 color ;
    vec3 vNormal = normalize(g2f.normal);
    if (vNormal.z < -0.01){
        //discard;//only draw visible surface.
    }
    color = GetColor(g2f.scalar);
    if (enableLight){
        vec3 L = normalize((e_lightPos - g2f.e_pos).xyz);
        intensity = max(dot(L, vNormal), 0);//dot(normalize(lightDir), -vNormal));
    }
    out_Color = color * intensity;
}
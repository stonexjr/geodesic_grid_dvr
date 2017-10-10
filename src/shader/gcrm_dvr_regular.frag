//#extension GL_EXT_gpu_shader4 : enable
#version 430 core

out vec4 out_Color;

uniform int nLayers;
uniform sampler2D uWorldColorMap;
uniform sampler2D uWorldAlphaMap;

in G2F{
	flat int triangle_id;
	smooth vec3 o_pos;
}g2f;

uniform mat4 inv_mvm;
uniform vec4 e_lightPos;
uniform bool enableLight;
uniform int  drawLine;
uniform int  drawWorldMap;

uniform vec2 split_range;
uniform vec3 vSizes;//volume grid dimension
uniform sampler1D tf;
uniform sampler2D texworldmap;
uniform sampler3D tex3dScalar;
uniform vec4 lightParam;// r:ambient, g:diffuse, b:specular, a:shininess
uniform float stepsize;
//uniform bool bPerspective;

vec3 epsilon = vec3(0.001);

out vec4 out_color;

#define BASESAMPLE (1.0/64.0)

vec4 getColor(float scalar, float ssize){
	vec4  color = texture(tf, scalar);
	float ratio = ssize / BASESAMPLE;
	float alpha = 1.0 - pow(1.0 - color.a, ratio);
	color.a = alpha;
	color.rgb = color.rgb * color.a;
	return color;
}

struct Light{
	float diffuse;
	float specular;
};

Light getLight(vec3 normal, vec3 lightDir, vec3 rayDir)
{
	Light light;
	float ambient = lightParam.r;
	float diffuse = lightParam.g * max(dot(lightDir, normal), dot(lightDir, -normal));
	//half angle
	vec3 half_angle = normalize(-rayDir + lightDir);
	float dotHV = max(dot(half_angle, normal), dot(half_angle, -normal));
	//float dotHV = dot(half, normal);
	float specular = 0.0;
	if (dotHV > 0.0){
		specular = lightParam.b * pow(dotHV, lightParam.a);
	}
	light.diffuse = ambient + diffuse;
	light.specular = specular;
	return light;
}

vec3 getNormal(vec3 texCoord){
	vec3 gradient;
	gradient.x = texture(tex3dScalar, texCoord + vec3(epsilon.x, 0, 0)).r
		- texture(tex3dScalar, texCoord + vec3(-epsilon.x, 0, 0)).r;
	gradient.y = texture(tex3dScalar, texCoord + vec3(0, epsilon.y, 0)).r
		- texture(tex3dScalar, texCoord + vec3(0, -epsilon.y, 0)).r;
	gradient.z = texture(tex3dScalar, texCoord + vec3(0, 0, epsilon.z)).r
		- texture(tex3dScalar, texCoord + vec3(0, 0, -epsilon.z)).r;
	if (length(gradient) > 0.0)//avoid division by zero or else black dots would show up!
		gradient = normalize(-gradient);
	return gradient;
}

#define PI_OVER_2 1.5707963
#define GLOBLE_RADIUS_OUTER PI_OVER_2
#define GLOBLE_RADIUS_INNER GLOBLE_RADIUS_OUTER*0.9
#define PI 3.1415926f
#define _2PI 6.2831852f
#define invPi 0.3183f
#define inv2Pi 0.1591f
#define RadiusToDegree 180.0f/PI

//http://www.cplusplus.com/reference/cmath/atan2/
vec2 SphericalXYZtoUV(in vec3 xyz){
	float r		= length(xyz);
	float theta = asin(xyz.y / r);
	float fi	= atan(xyz.x, xyz.z);//[-pi,+pi]
	return vec2(fi * inv2Pi + 0.5, theta * invPi + 0.5);//normalize to [0,1]
}
//http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
//return vec3.x=fnear, vec3.y=fFar, vec3.z=bool : miss or hit.
//make sure ray_o is normalized!!!
//radius2: R^2
//Be careful: if ray_o is inside sphere, vec3.x < 0, use this value at your
//own discretion.
vec3 raySphareIntersection(vec3 center, float radius2, vec3 ray_o, vec3 ray_dir)
{
	float P0Pcenter2 = length(ray_o - center);
	P0Pcenter2 = P0Pcenter2*P0Pcenter2;
	//float A=dot(ray_dir,ray_dir)=1;
	float B = 2.0 * dot(ray_dir, (ray_o - center));
	float C = P0Pcenter2 - radius2;
	float disc = B*B - 4.0 * C;
	if (disc < 0.0)
	{//discriminant < 0, ray misses sphere
		return vec3(0, 0, 0);
	}
	float distSqrt = sqrt(disc);
	float q = 0;
	vec3  hitInfo = vec3(0, 0, 1);
	if (B < 0.0)
	{
		q = (-B + distSqrt)*0.5;
		hitInfo.x = C / q;//t0
		hitInfo.y = q;//t1:=q/A
	}
	else{
		q = (-B - distSqrt)*0.5;
		hitInfo.x = q;//t0:=q/A
		hitInfo.y = C / q;//t1
	}
	// if t1 is less than zero, the object is in the ray's negative
	// direction and consequently the ray misses the sphere.
	if (hitInfo.y < 0)
		hitInfo.z = 0;
	return hitInfo;
}

mat3 jacobianMatrix(vec3 xyz)
{
	float len = length(xyz);
	float x = xyz.z, y = xyz.y, z = xyz.z;
	float x2z2 = x*x + z*z;
	float sqrtx2z2 = sqrt(x2z2);
	float lensqr = len*len;
	float invlen = 1.0 / len;
	//create matrix in column major.
	mat3 jm = mat3(
		z / (_2PI*(x2z2)), 0.0, - x / (_2PI*(x2z2)),//det[phi/x],det[phi/y],det[phi/z]
		-1.0 * x*y / (PI*sqrtx2z2*lensqr), sqrtx2z2 / (PI*lensqr), -1.0 * y*z / (PI*sqrtx2z2*lensqr),//det[theta/x],det[theta/y],det[theta/z]
		x*invlen, y*invlen, z*invlen//det[r/x],det[r/y],det[r/z]
		);
	return jm;
}

void main(void)
{
	vec3 position_o = g2f.o_pos;
	//ray direction
	vec3 rayStart = position_o;
	vec3 dir;
	bool bPerspective = true;
	if (bPerspective)
	{//perspective projection: ray starts from eye/camera.
		//Convert camera position in camera space, which is (0,0,0) to object space/
		vec3 eye_o = (inv_mvm * vec4(0, 0, 0, 1)).xyz;

		dir = (position_o- eye_o);
	}
	else{
		dir = (inv_mvm * vec4(0, 0, -1, 0)).xyz;
	}
	dir = normalize(dir);

	float ssize = max(0.0001, stepsize);
	vec3 vSizes = vec3(textureSize(tex3dScalar, 0));
	epsilon = vec3(1.0) / vSizes;
	//----
	vec3 hitInfoOuter, hitInfoInner;
	hitInfoOuter = raySphareIntersection(vec3(0.0), GLOBLE_RADIUS_OUTER*GLOBLE_RADIUS_OUTER, rayStart, dir);
	hitInfoInner = raySphareIntersection(vec3(0.0), GLOBLE_RADIUS_INNER*GLOBLE_RADIUS_INNER, rayStart, dir);
	float t = hitInfoOuter.x, texit = hitInfoOuter.y;
	if (hitInfoInner.z>0.0)
	{//ray also hit inner sphere(i.e. Earth terrain)
		texit = hitInfoInner.x;
	}
	vec4  accum_color = vec4(0);
	float accum_alpha = 0;//accumulated opacity

	float scalar = 0.0;
	float gradient = 0.0;
	//float thickness = GLOBLE_RADIUS*0.1 / float(nLayers);
	float thickness = GLOBLE_RADIUS_OUTER - GLOBLE_RADIUS_INNER;//total thickness of domain layers.

	vec3 lightDir = normalize(vec3(6.0));
	vec3 cartianCoord = rayStart + dir*t;
	vec3 texCoord = vec3(0.0);

	while (t <= texit && accum_alpha < 0.98)
	{
		cartianCoord = rayStart + dir*t;
		texCoord.xy = SphericalXYZtoUV(cartianCoord);
		if (texCoord.x >= split_range.x &&
			texCoord.x <= split_range.y)
		{
			t = t + ssize;
			continue;
		}
		texCoord.z = (length(cartianCoord) - (GLOBLE_RADIUS_OUTER - thickness)) / thickness;
		scalar = texture(tex3dScalar, texCoord).r;
		vec4 color = getColor(scalar, ssize);
		//if (color.a > 0.001)
		{
			if (enableLight)
			{
				vec3 normal = getNormal(texCoord);
				//Change-of-variable: convert gradient is Cartesian coordinate to texture coordinates.
				/*
				if (length(normal)>0.0)
				{
					mat3 jacobMat3 = jacobianMatrix(cartianCoord);
					normal = normalize(jacobMat3 * normal);
				}
				*/
				Light light = getLight(normal, lightDir, dir);
				color.rgb = color.rgb * light.diffuse + color.a * light.specular;
			}
			//accum_color = (1.0 - accum_color.a)*color + accum_color;
			//fma(a,b,c): computes a*b+c in a single operation.
			accum_color = fma(vec4(1.0 - accum_color.a), color, accum_color);
		}
		t = t + ssize;
	}
	//blend with world map
	if (hitInfoInner.z>0)
	{
		vec4 color = texture(texworldmap, texCoord.xy);
		accum_color = fma(vec4(1.0 - accum_color.a), color, accum_color);
	}
	out_color = accum_color;
}
//#extension GL_EXT_gpu_shader4 : enable
#version 430 core

out vec4 out_Color;

uniform int nLayers;
uniform sampler2D uWorldColorMap;
uniform sampler2D uWorldAlphaMap;
uniform sampler1D tf;

in G2F{
	flat int triangle_id;
	smooth vec3 o_pos;
}g2f;

uniform mat4 inv_mvm;
uniform vec4 e_lightPos;
uniform int enableLight;
uniform int drawLine;
uniform int drawWorldMap;
uniform float stepsize;


#define  PI_OVER_2 1.5707963
#define  _2_OVER_PI 0.63661978
#define  PI 3.1415926
#define  INV_PI 0.31830989
#define  GLOBLE_RADIUS PI_OVER_2
#define Pi    3.14159
#define invPi 0.3183
#define inv2Pi 0.1591
#define alphaThreshold 0.1


struct Ray{
	vec3 o;
	vec3 d;
};

vec4 gcrm_GetColor(float scalar)
{
	vec4 tempColor = texture(tf, scalar);
	//#define  d_fSampleSpacing 1.0/64.0
#define  d_fSampleSpacing 0.015625

	float alpha = tempColor.w;
	//gamma correction
	alpha = 1 - pow((1 - alpha), stepsize / d_fSampleSpacing);
	alpha /= GLOBLE_RADIUS;
	tempColor.x = tempColor.x * alpha;
	tempColor.y = tempColor.y * alpha;
	tempColor.z = tempColor.z * alpha;
	tempColor.w = alpha;
	return tempColor;
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

struct Light{
	float diffuse;
	float specular;
};

Light getLight(vec3 normal, vec3 lightDir, vec3 rayDir){
	Light light;
	float ambient = 0.8;// lightParam.r;
	float diffuse = max(dot(lightDir, normal), dot(lightDir, -normal)) * 0.7;// lightParam.g *;
	//half angle
	vec3 halfAngle = normalize(-rayDir + lightDir);
	float dotHV = max(dot(halfAngle, normal), dot(halfAngle, -normal));
	//float dotHV = dot(half, normal);
	float specular = 0.0;
	if (dotHV > 0.0){
		specular = 0.5 * pow(dotHV, 20);//lightParam.b * pow(dotHV, lightParam.a);
	}
	light.diffuse = ambient + diffuse;
	light.specular = specular;
	return light;
}

void main(){
	float intensity = 1.0;
	//perform ray-casting through triangle mesh.
	vec3 o_eye = (inv_mvm * vec4(0, 0, 0, 1.0)).xyz;
	Ray ray;
	ray.o = o_eye;
	ray.d = normalize(g2f.o_pos - o_eye);

	vec3 hitRec = raySphareIntersection(vec3(0.0), GLOBLE_RADIUS * GLOBLE_RADIUS, ray.o, ray.d);
	vec3 dir = ray.d;
	vec3 rayStart = ray.o + dir * hitRec.x;
	float texit = hitRec.y - hitRec.x;

	vec4 accum_color = vec4(0);
	float t = 0;
	while (t <= texit && accum_color.a < 1.0)
	{
		vec3 cur_pos = rayStart + dir*t;
		//vec3 texCoord = cur_pos;
		float scalar = distance(cur_pos, vec3(0.0)) / GLOBLE_RADIUS;
		vec4  color = gcrm_GetColor(scalar);//getColor(scalar, ssize);
		{
			if (true)
			{
				vec3 normal = normalize(cur_pos);//getNormal(texCoord);
				vec3 lightDir = normalize(vec3(6.0));
				Light light = getLight(normal, lightDir, dir);
				color.rgb = color.rgb * light.diffuse + color.a * light.specular;
			}
			//accum_color = (1.0 - accum_color.a)*color + accum_color;
			//fma(a,b,c): computes a*b+c in a single operation.
			accum_color = fma(vec4(1.0 - accum_color.a), color, accum_color);
		}
		t = t + stepsize;
	}
	out_Color = accum_color;
}
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

out vec4 out_Color;

uniform int nLayers;
uniform sampler1D tf;

in G2F{
    flat int triangle_id;
    smooth vec3 o_pos;
}g2f;

uniform mat4 inv_mvm;
uniform vec4 e_lightPos;
uniform bool enableLight;
uniform float stepsize;
uniform vec4 material;

//connectivity
uniform samplerBuffer  center_lat;
uniform samplerBuffer  center_lon;
uniform isamplerBuffer corner_cells;
uniform isamplerBuffer corner_edges;
uniform isamplerBuffer edge_cells;
uniform isamplerBuffer edge_corners;
uniform isamplerBuffer cell_corners;
uniform samplerBuffer  center_val;
//symbolic definition of dual triangle mesh
#define tri_corner_id1x3  corner_cells
#define tri_corner_val1x1 center_val
#define CLIMATE_VALS_VAR  center_val
#define TRIANGLE_TO_EDGES_VAR corner_edges
#define EDGE_CORNERS_VAR edge_cells
#define CORNERS_LAT_VAR  center_lat 
#define CORNERS_LON_VAR  center_lon 
#define FACEID_TO_EDGEID gcrm_faceId_to_edgeId
#define EDGE_TO_TRIANGLES_VAR edge_corners
#define CORNER_TO_TRIANGLES_VAR cell_corners

#define FLOAT_MAX  3.402823466e+38F
#define FLOAT_MIN -2.402823466e+36F
#define TOTAL_LAYERS 60
#define THICKNESS GLOBLE_RADIUS*0.1/float(TOTAL_LAYERS)
#define RealType float
#define RealType3 vec3 

#define  PI_OVER_2 1.5707963
#define  GLOBLE_RADIUS PI_OVER_2

//TODO: is it memory effecient?
int	d_gcrm_faceCorners[24] = {
    0, 1, 2,  3, 4, 5,//top 0 and bottom 1
    4, 2, 1,  4, 5, 2,//font 2,3
    5, 0, 2,  5, 3, 0,//right 4,5
    0, 3, 1,  3, 4, 1//left 6,7
};

vec4 GetColor(in float scale){
    vec4 tempColor = texture(tf, scale);
    return tempColor;
}

vec4 e_light_pos = vec4(1, 1, 5, 1);

struct Ray{
    vec3 o;
    vec3 d;
};

struct HitRec{
    float t;// t value along with hitted face
    int hitFaceid;//hitted face id
    int nextlayerId;
};

struct GCRMPrism{
    uint m_prismId;
    int  m_iLayer;
    vec3 vtxCoordTop[3];//3 Top(T) and 3 Bottom(B) vertex coordinates
    vec3 vtxCoordBottom[3];//3 Top(T) and 3 Bottom(B) vertex coordinates
    int m_idxEdge[3];
    int idxVtx[3];//triangle vertex index is equivalent to hexagon cell index
};
#define DOUBLE_ERROR 1.0e-8
bool rayIntersectsTriangleDouble(dvec3 p, dvec3 d,
    dvec3 v0, dvec3 v1, dvec3 v2, inout double t)
{
    dvec3 e1, e2, h, s, q;
    double a, f, u, v;
    //float error = 1.0e-4;//0.005f;
    e1 = v1 - v0;
    e2 = v2 - v0;
    //crossProduct(h, d, e2);
    h = cross(d, e2);
    a = dot(e1, h);//innerProduct(e1, h);

    if (a > -DOUBLE_ERROR && a < DOUBLE_ERROR)
        return(false);

    f = 1.0 / a;
    s = p - v0;//_vector3d(s, p, v0);
    u = f * dot(s, h);//(innerProduct(s, h));

    if (u < -DOUBLE_ERROR || u >(1.0 + DOUBLE_ERROR))
        return(false);

    q = cross(s, e1);//crossProduct(q, s, e1);
    v = f * dot(d, q);//innerProduct(d, q);

    if (v < -DOUBLE_ERROR || u + v >(1.0 + DOUBLE_ERROR))
        return(false);

    // at this stage we can compute t to find out where
    // the intersection point is on the line
    t = f * dot(e2, q);//innerProduct(e2, q);

    if (t > DOUBLE_ERROR)//ray intersection
        return(true);
    else // this means that there is a line intersection
        // but not a ray intersection
        return (false);
}

#define FLOAT_ERROR 1.0E-8

bool rayIntersectsTriangleFloat(vec3 p, vec3 d,
    vec3 v0, vec3 v1, vec3 v2, inout float t)
{
    vec3 e1, e2, h, s, q;
    float a, f, u, v;
    //float error = 1.0e-4;//0.005f;
    e1 = v1 - v0;
    e2 = v2 - v0;
    //crossProduct(h, d, e2);
    h = cross(d, e2);
    a = dot(e1, h);//innerProduct(e1, h);

    if (a > -FLOAT_ERROR && a < FLOAT_ERROR)
        return(false);

    f = 1.0f / a;
    s = p - v0;//_vector3d(s, p, v0);
    u = f * dot(s, h);//(innerProduct(s, h));

    if (u < -FLOAT_ERROR || u >(1.0f + FLOAT_ERROR))
        return(false);

    q = cross(s, e1);//crossProduct(q, s, e1);
    v = f * dot(d, q);//innerProduct(d, q);

    if (v < -FLOAT_ERROR || u + v >(1.0f + FLOAT_ERROR))
        return(false);

    // at this stage we can compute t to find out where
    // the intersection point is on the line
    t = f * dot(e2, q);//innerProduct(e2, q);

    if (t > FLOAT_ERROR)//ray intersection
        return(true);
    else // this means that there is a line intersection
        // but not a ray intersection
        return (false);
}

bool ReloadVtxInfo(in int triangle_id, in int ilayer, inout GCRMPrism prism)
{
    prism.m_prismId = triangle_id;
    prism.m_iLayer = ilayer;

    //TODO:maybe use local vec3 idxVtx[3]
    prism.idxVtx[0] = -1;
    prism.idxVtx[1] = -1;
    prism.idxVtx[2] = -1;

    //load first edge
    //index of first edge of triangle
    ivec3 idxEdges = texelFetch(TRIANGLE_TO_EDGES_VAR, triangle_id).xyz;
    int idxEdge = idxEdges.x;//TRIANGLE_TO_EDGES_VAR[m_prismId * 3 + 0];
    prism.m_idxEdge[0] = idxEdge;
    //index of start corner of this  edge
    ivec2 cornerIdxs = texelFetch(EDGE_CORNERS_VAR, idxEdge).xy;
    int iS = cornerIdxs.x;//EDGE_CORNERS_VAR[idxEdge * 2];	   
    //index of end corner of this  edge
    int iE = cornerIdxs.y;//EDGE_CORNERS_VAR[idxEdge * 2 + 1];
    prism.idxVtx[0] = iS;
    prism.idxVtx[1] = iE;
    //int edge1S = iS;
    int edge1E = iE;

    //load second edge.
    idxEdge = idxEdges.y;//TRIANGLE_TO_EDGES_VAR[m_prismId * 3 + 1];//index of second edge.
    prism.m_idxEdge[1] = idxEdge;
    //index of start corner of second edge
    cornerIdxs = texelFetch(EDGE_CORNERS_VAR, idxEdge).xy;
    iS = cornerIdxs.x;//EDGE_CORNERS_VAR[idxEdge * 2];		
    //index of end corner of second edge
    iE = cornerIdxs.y;//EDGE_CORNERS_VAR[idxEdge * 2 + 1];	

    bool normalCase = (edge1E != iS) && (edge1E != iE);
    if (iS != prism.idxVtx[0] && iS != prism.idxVtx[1]){//find the index of the third corner.
        prism.idxVtx[2] = iS;
    }
    else{
        prism.idxVtx[2] = iE;
    }
    //index of third edge.
    idxEdge = idxEdges.z;//TRIANGLE_TO_EDGES_VAR[m_prismId * 3 + 2];
    prism.m_idxEdge[2] = idxEdge;
    if (!normalCase){
        //swap m_idxEdge[1] and m_idxEdge[2]
        prism.m_idxEdge[1] ^= prism.m_idxEdge[2];
        prism.m_idxEdge[2] ^= prism.m_idxEdge[1];
        prism.m_idxEdge[1] ^= prism.m_idxEdge[2];
    }
    //load vertex index based on the edges's info
    //end corners on the first edge

    float lon[3], lat[3];// longitude and latitude of three corners.
    float _cos_sin[3], _sin[3], _cos_cos[3];
    float maxR = GLOBLE_RADIUS - THICKNESS*ilayer;// Radius of top triangle in current cell
    float minR = maxR - THICKNESS;				 // Radius of bottom triangle in current cell
    for (int i = 0; i < 3; ++i)
    {//for each corner index of the triangle(specified by prismId)
        int idxCorner = prism.idxVtx[i];//TRIANGLE_TO_CORNERS_VAR[m_prismId*3+i];
        lat[i] = texelFetch(CORNERS_LAT_VAR, idxCorner).r;//CORNERS_LAT_VAR[idxCorner];
        lon[i] = texelFetch(CORNERS_LON_VAR, idxCorner).r;//CORNERS_LON_VAR[idxCorner];
        _cos_sin[i] = cos(lat[i])*sin(lon[i]);
        _sin[i] = sin(lat[i]);
        _cos_cos[i] = cos(lat[i])*cos(lon[i]);
    }

    prism.vtxCoordTop[0] = vec3(maxR*_cos_sin[0], maxR*_sin[0], maxR*_cos_cos[0]);
    prism.vtxCoordTop[1] = vec3(maxR*_cos_sin[1], maxR*_sin[1], maxR*_cos_cos[1]);
    prism.vtxCoordTop[2] = vec3(maxR*_cos_sin[2], maxR*_sin[2], maxR*_cos_cos[2]);
    prism.vtxCoordBottom[0] = vec3(minR*_cos_sin[0], minR*_sin[0], minR*_cos_cos[0]);
    prism.vtxCoordBottom[1] = vec3(minR*_cos_sin[1], minR*_sin[1], minR*_cos_cos[1]);
    prism.vtxCoordBottom[2] = vec3(minR*_cos_sin[2], minR*_sin[2], minR*_cos_cos[2]);

    return true;
}

//Return adjacent mesh cell(which is triangle in the remeshed GCRM mesh) id
// which shares with current triangle(specified by curTriangleId) the edge belongs
// to the face( denoted by faceId)
int getAdjacentCellIdGCRM(inout GCRMPrism prism, int faceId, int prevTriangleId){
    if (prism.m_iLayer == TOTAL_LAYERS - 2 && faceId == 1){
        return -1; //we reached the deepest layers, no more layers beyond current layer.
    }
    else if (faceId == 0 || faceId == 1){
        return int(prism.m_prismId);//curTriangleId;
    }

    const int gcrm_faceId_to_edgeId[8] = { -1, -1, 2, 2, 1, 1, 0, 0 };
    int edgeId = FACEID_TO_EDGEID[faceId];
    int idxEdge = prism.m_idxEdge[edgeId];
    ivec2 nextTriangleIds = texelFetch(EDGE_TO_TRIANGLES_VAR, idxEdge).xy;//EDGE_TO_TRIANGLES_VAR[idxEdge * 2];
    if (nextTriangleIds.x == prism.m_prismId)// curTriangleId)
    {// nextTriangleId = texelFetch(EDGE_TO_TRIANGLES_VAR, idxEdge * 2 + 1).r;
        return nextTriangleIds.y;
    }
    return nextTriangleIds.x;
}

#define INTERSECT_DOUBLE_PRECISION
#define FACEID_TO_CORNERID d_gcrm_faceCorners

int rayPrismIntersection(inout GCRMPrism prism, in Ray r, in int prevCellId, inout HitRec tInRec,
    inout HitRec tOutRec, inout int nextCellId)
{
    nextCellId = -1;//assume no next prism to shot into.
    int nHit = 0; int nFaces = 8;
    tOutRec.hitFaceid = -1;//initialize tOutRec
    tOutRec.t = -1.0f;
    double min_t = FLOAT_MAX, max_t = -1.0f;
    vec3 vtxCoord[6];
    vtxCoord[0] = prism.vtxCoordTop[0];
    vtxCoord[1] = prism.vtxCoordTop[1];
    vtxCoord[2] = prism.vtxCoordTop[2];
    vtxCoord[3] = prism.vtxCoordBottom[0];
    vtxCoord[4] = prism.vtxCoordBottom[1];
    vtxCoord[5] = prism.vtxCoordBottom[2];

    for (int idxface = 0; idxface < nFaces/*=8*/; ++idxface)
    {//8 faces
        vec3 v0 = vtxCoord[FACEID_TO_CORNERID[idxface * 3 + 0]];
        vec3 v1 = vtxCoord[FACEID_TO_CORNERID[idxface * 3 + 1]];
        vec3 v2 = vtxCoord[FACEID_TO_CORNERID[idxface * 3 + 2]];

#ifdef INTERSECT_DOUBLE_PRECISION
        double t = 0.0;
        dvec3 rayO = dvec3(r.o);
        dvec3 rayD = dvec3(r.d);
        dvec3 vtxTB0 = dvec3(v0);
        dvec3 vtxTB1 = dvec3(v1);
        dvec3 vtxTB2 = dvec3(v2);
        bool bhit = rayIntersectsTriangleDouble(rayO, rayD,
                    vtxTB0, vtxTB1, vtxTB2, t);
#else
        float t = 0.0;
        vec3 rayO = vec3(r.o);
        vec3 rayD = vec3(r.d);
        vec3 vtxTB0 = vec3(v0);
        vec3 vtxTB1 = vec3(v1);
        vec3 vtxTB2 = vec3(v2);
        bool bhit = rayIntersectsTriangleFloat(rayO, rayD,
                    vtxTB0, vtxTB1, vtxTB2, t);
#endif
        if (bhit)
        {
            nHit++;

            if (min_t > t){
                min_t = t;
                tInRec.t = float(t);
                tInRec.hitFaceid = idxface;
            }
            if (max_t < t){
                max_t = t;
                tOutRec.t = float(t);
                tOutRec.hitFaceid = idxface;
                if (idxface == 1){
                    tOutRec.nextlayerId = prism.m_iLayer + 1;//the next prism to be traversed is in the lower layer
                }
                else if (idxface == 0){
                    tOutRec.nextlayerId = prism.m_iLayer - 1;//the next prism to be traversed is in the upper layer
                }
                else{
                    tOutRec.nextlayerId = prism.m_iLayer;
                }
            }
        }
    }

    if (nHit == 2)
    {
        nextCellId = getAdjacentCellIdGCRM(prism, tOutRec.hitFaceid, prevCellId);
    }
    else
    {//Special case when ray hit on the edge.
        nextCellId = -1;
    }

    return nHit;
}


void GetAs0(inout GCRMPrism prism, inout RealType A[12])
{
    float x1 = (prism.vtxCoordTop[0].x);
    float y1 = (prism.vtxCoordTop[0].y);
    float z1 = (prism.vtxCoordTop[0].z);

    float x2 = (prism.vtxCoordTop[1].x);
    float y2 = (prism.vtxCoordTop[1].y);
    float z2 = (prism.vtxCoordTop[1].z);

    float x3 = (prism.vtxCoordTop[2].x);
    float y3 = (prism.vtxCoordTop[2].y);
    float z3 = (prism.vtxCoordTop[2].z);

    A[1] = (+x3*y1 - x1*y3);
    A[2] = (+x3*z1 - x1*z3);
    A[3] = (+y3*z1 - y1*z3);
    A[4] = (-x2*y1 + x1*y2);
    A[5] = (-x2*z1 + x1*z2);
    A[6] = (-y2*z1 + y1*z2);
    A[7] = (-x2*y1 + x3*y1 + x1*y2 - x3*y2 - x1*y3 + x2*y3);//x3 * (y1 - y2) + x1 * (y2 - y3) + x2 * (-y1 + y3);//(-x2*y1 + x3*y1 + x1*y2 - x3*y2 - x1*y3 + x2*y3);
    A[8] = (-x2*z1 + x3*z1 + x1*z2 - x3*z2 - x1*z3 + x2*z3);//x3 * (z1 - z2) + x1 * (z2 - z3) + x2 * (-z1 + z3);//(-x2*z1 + x3*z1 + x1*z2 - x3*z2 - x1*z3 + x2*z3);
    A[9] = (-y2*z1 + y3*z1 + y1*z2 - y3*z2 - y1*z3 + y2*z3);//y3 * (z1 - z2) + y1 * (z2 - z3) + y2 * (-z1 + z3);//(-y2*z1 + y3*z1 + y1*z2 - y3*z2 - y1*z3 + y2*z3);
    A[10] = (x3*y2*z1);
    A[11] = (-x2*y3*z1 - x3*y1*z2 + x1*y3*z2 + x2*y1*z3 - x1*y2*z3);
}

void GetBs(
    inout GCRMPrism prism,
    inout RealType A[12], inout RealType3 fTB[2], inout RealType B[8],
    inout RealType OP1, inout RealType OP4)
{

    OP1 = length(prism.vtxCoordTop[0]);
    OP4 = length(prism.vtxCoordBottom[0]);
    B[0] = 1.0f / ((A[10] + A[11])*(OP1 - OP4));//BUG-->//1.0f/((A[1]+A[11])*OP1 - OP4);//
    B[1] = (A[3] + A[6] - A[9]) * fTB[0].x - A[3] * fTB[0].y - A[6] * fTB[0].z + (-A[3] - A[6] + A[9])* fTB[1].x + A[3] * fTB[1].y + A[6] * fTB[1].z;
    //(A3 + A6 - A9) V1			- A3 V2			- A6 V3			 + (-A3 - A6 + A9) V4			+ A3 V5			+ A6 V6)
    B[2] = (A[1] + A[4] - A[7]) * fTB[0].x - A[1] * fTB[0].y - A[4] * fTB[0].z + (-A[1] - A[4] + A[7])* fTB[1].x + A[1] * fTB[1].y + A[4] * fTB[1].z;
    // (A1 + A4 - A7) V1				- A1 V2			- A4 V3			 + (-A1 - A4 + A7) V4			+ A1 V5			+ A4 V6)
    B[3] = (-A[2] - A[5] + A[8])* fTB[0].x + A[2] * fTB[0].y + A[5] * fTB[0].z + (A[2] + A[5] - A[8])* fTB[1].x - A[2] * fTB[1].y - A[5] * fTB[1].z;
    // (-A2 - A5 + A8) V1			+ A2 V2			+ A5 V3			 + (A2 + A5 - A8) V4				- A2 V5			- A5 V6
    B[4] = fTB[0].x - fTB[0].y;//V1-V2
    B[5] = fTB[0].x - fTB[0].z;//V1-V3
    B[6] = fTB[1].x - fTB[1].y;//V4-V5
    B[7] = fTB[1].x - fTB[1].z;//V4-V6
}

void GetScalarValue(inout GCRMPrism prism, inout RealType3 scalars[2]){
    //int dim3_size = nLayers;//CLIMATE_VALS_DIMSIZES(2);//layer dimension
    int time = 0;//?? we haven't get into time variant data.

    for (int iFace = 0; iFace < 2; ++iFace)
    {
        int layerId = prism.m_iLayer + iFace;
        scalars[iFace].x = texelFetch(CLIMATE_VALS_VAR, prism.idxVtx[0] * nLayers + layerId).r;//CLIMATE_VALS_VAR[idxVtx[0] * nLayers + (m_iLayer + iFace)];
        scalars[iFace].y = texelFetch(CLIMATE_VALS_VAR, prism.idxVtx[1] * nLayers + layerId).r; //CLIMATE_VALS_VAR[idxVtx[1] * nLayers + (m_iLayer + iFace)];
        scalars[iFace].z = texelFetch(CLIMATE_VALS_VAR, prism.idxVtx[2] * nLayers + layerId).r; //CLIMATE_VALS_VAR[idxVtx[2] * nLayers + (m_iLayer + iFace)];
    }
}

void GetUVT(in RealType3 O, in RealType3 Q, inout RealType A[12],
    inout RealType u, inout RealType v, inout RealType t1)
{
    RealType3 QO = (Q - O);//*Factor;
    RealType denominator = (A[9] * QO.x - A[8] * QO.y + A[7] * QO.z);
    u = (A[3] * QO.x - A[2] * QO.y + A[1] * QO.z) / denominator;
    v = (A[6] * QO.x - A[5] * QO.y + A[4] * QO.z) / denominator;
}

float GetInterpolateValue2(inout GCRMPrism prism, in const float u, in const float v,
    in const vec3 Q, inout vec3 fT, inout vec3 fB)
{
    vec3 baryCoord = vec3(1.0 - u - v, u, v);
    vec3 m1 = baryCoord.x * prism.vtxCoordTop[0] + baryCoord.y * prism.vtxCoordTop[1]
        + baryCoord.z * prism.vtxCoordTop[2];
    vec3 m2 = baryCoord.x * prism.vtxCoordBottom[0] + baryCoord.y * prism.vtxCoordBottom[1]
        + baryCoord.z * prism.vtxCoordBottom[2];

    float scalar_m1 = dot(baryCoord, fT);
    float scalar_m2 = dot(baryCoord, fB);
    float t3 = length(Q - m2) / length(m1 - m2);
    float lerpedVal = mix(scalar_m2, scalar_m1, t3);//lerp();

    return lerpedVal;
}

//*************************************************************************
//Analytical solution of normal vector. 
//Refer to the appendix section of my dissertation for detailed derivation.
//*************************************************************************
RealType3 GetNormal(const RealType3 position, const RealType A[12],
    const RealType B[8], const RealType OP1, const RealType OP4){
    RealType delx = 0.0f, dely = 0.0f, delz = 0.0f;//partial derivative of scalar value at sampling point.
    RealType inv_denom = 1.0f / (A[9] * position.x - A[8] * position.y + A[7] * position.z);
    RealType C0 = B[0] * OP1*(A[9] * position.x - A[8] * position.y + A[7] * position.z)*(B[1] * position.x + B[3] * position.y + B[2] * position.z);
    //B10 OP1 (A9 qx			- A8 qy				+ A7 qz)		 (B11 qx			+ B13 qy		 + B12 qz)
    RealType temp = (A[10] + A[11])*OP4 + OP1*(A[9] * position.x - A[8] * position.y + A[7] * position.z);
    //(A10 + A11) OP4	 + OP1 (A9 qx			- A8 qy				+ A7 qz)
    RealType C1 = B[6] - B[0] * (B[4] - B[6])*temp;
    //B16 - B10 (B14 - B16)
    RealType C2 = B[7] - B[0] * (B[5] - B[7])*temp;
    //B17 - B10 (B15 - B17)

    delx = (inv_denom*inv_denom)*
        (A[9] * C0 +
        C1*((A[3] * A[8] - A[2] * A[9])*position.y + (-A[3] * A[7] + A[1] * A[9])*position.z) +
        C2*((A[6] * A[8] - A[5] * A[9])*position.y + (-A[6] * A[7] + A[4] * A[9])*position.z)
        );
    //A9 C0 +
    //		C1 ((A3 A8		- A2 A9) qy				+ (-A3 A7		+ A1 A9) qz) +
    //		C2 ((A6 A8		- A5 A9) qy				+ (-A6 A7		+ A4 A9) qz)
    dely = (inv_denom*inv_denom)*
        (-A[8] * C0 +
        C1*((-A[3] * A[8] + A[2] * A[9])*position.x + (A[2] * A[7] - A[1] * A[8])*position.z) +
        C2*((-A[6] * A[8] + A[5] * A[9])*position.x + (A[5] * A[7] - A[4] * A[8])*position.z)
        );
    //-A8 C0 +
    //		C1 ((-A3 A8			+ A2 A9) qx			+ (A2 A7		- A1 A8) qz) +
    //		C2 ((-A6 A8			+ A5 A9) qx			+ (A5 A7		- A4 A8) qz)
    delz = (inv_denom*inv_denom)*
        (A[7] * C0 +
        C1*((A[3] * A[7] - A[1] * A[9])*position.x + (-A[2] * A[7] + A[1] * A[8])*position.y) +
        C2*((A[6] * A[7] - A[4] * A[9])*position.x + (-A[5] * A[7] + A[4] * A[8])*position.y)
        );
    //A7 C0 +
    // C1 ((A3 A7		- A1 A9) qx				+ (-A2 A7		+ A1 A8) qy) +
    // C2 ((A6 A7		- A4 A9) qx				+ (-A5 A7		+ A4 A8) qy)
    return normalize(vec3(delx, dely, delz));
}

void ComputeVerticesNormal(inout GCRMPrism prism, inout RealType3 vtxNormals[3])
{
    int nNeighbors = 6;//CORNER_TO_TRIANGLES_DIMSIZES(1);
    //for each one of the three corners of top face of current prism.
    //compute their shared normals respectively.
    for (int iCorner = 0; iCorner < 3; iCorner++){
        int idxCorner = prism.idxVtx[iCorner];//TRIANGLE_TO_CORNERS_VAR[m_prismId*3+iCorner];
        //foreach 6 prisms(including current prism and 5 neighbor prisms).
        RealType3 avgNormal = vec3(0.0f);
        for (int iPrism = 0; iPrism < nNeighbors; iPrism++){//weighted normal
            int id = texelFetch(CORNER_TO_TRIANGLES_VAR, idxCorner * nNeighbors + iPrism).x;//CORNER_TO_TRIANGLES_VAR[idxCorner * nNeighbors + iPrism];//6 + iPrism];
            GCRMPrism curPrismHitted;// (id, m_iLayer);
            ReloadVtxInfo(id, prism.m_iLayer, curPrismHitted);
            //find the vertex in curPrism whose global index==idxCorner
            int i = 0;
            for (; i < 3; i++){
                if (curPrismHitted.idxVtx[i] == iCorner)//idxCorner)// iCorner)//bug detected!!!
                    break;
            }
            RealType A[12];
            GetAs0(curPrismHitted, A);
            RealType3 fTB[2];
            GetScalarValue(curPrismHitted, fTB);
            RealType OP1, OP4, B[8];
            GetBs(curPrismHitted, A, fTB, B, OP1, OP4);
            avgNormal += GetNormal(curPrismHitted.vtxCoordTop[i], A, B, OP1, OP4);
        }
        vtxNormals[iCorner] = (avgNormal / 6.0f);
    }
}

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
    float ambient = material.r;
    float diffuse = material.g * max(dot(lightDir, normal), dot(lightDir, -normal));
    //half angle
    vec3 halfAngle = normalize(-rayDir + lightDir);
    float dotHV = max(dot(halfAngle, normal), dot(halfAngle, -normal));
    //float dotHV = dot(half, normal);
    float specular = 0.0;
    if (dotHV > 0.0){
        specular = material.b * pow(dotHV, material.a);
    }
    light.diffuse = ambient + diffuse;
    light.specular = specular;
    return light;
}

Light getLightAvg(vec2 uv, vec3 normal3[3], vec3 lightDir, vec3 rayDir){
    Light light;
    float ambient = material.r;
    vec3  normal  = normalize(normal3[0] * (1 - uv.x - uv.y) + normal3[1] * uv.x + normal3[2] * uv.y);
    float diffuse = material.g * max(dot(lightDir, normal), dot(lightDir, -normal)) ;
    //half angle
    vec3 halfAngle = normalize(-rayDir + lightDir);
    float dotHV = max(dot(halfAngle, normal), dot(halfAngle, -normal));
    //float dotHV = dot(half, normal);
    float specular = 0.0;
    if (dotHV > 0.0){
        specular = material.b * pow(dotHV, material.a);
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
    int triangle_id = g2f.triangle_id;

    HitRec tInHitRecord, tOutHitRecord, tmpInRec, tmpOutRec;
    tInHitRecord.hitFaceid = -1;
    tInHitRecord.t = FLOAT_MAX;
    tInHitRecord.nextlayerId = -1;

    tOutHitRecord.hitFaceid = -1;
    tOutHitRecord.t = FLOAT_MIN;
    tOutHitRecord.nextlayerId = -1;
    tmpInRec.t = FLOAT_MAX;
    tmpOutRec.t = FLOAT_MIN;
    tmpOutRec.nextlayerId = -1;

    int nHit = 0;
    int nextCellId = -1;
    int tmpNextCellId = -1;
    int prevNhits = 0;
    uint curPrismHittedId = triangle_id;

    GCRMPrism curPrismHitted;
    curPrismHitted.idxVtx[0] = -1;
    curPrismHitted.idxVtx[1] = -1;
    curPrismHitted.idxVtx[2] = -1;
    ReloadVtxInfo(int(curPrismHittedId), 0, curPrismHitted);

    int tmpNhit = rayPrismIntersection(curPrismHitted, ray, prevNhits, tmpInRec, tmpOutRec, tmpNextCellId);

    if (tmpNhit>0){
        nHit = tmpNhit;
        nextCellId = tmpNextCellId;
        curPrismHittedId = triangle_id;
        tInHitRecord = (tInHitRecord.t > tmpInRec.t) ? tmpInRec : tInHitRecord;
        tOutHitRecord = (tOutHitRecord.t < tmpOutRec.t) ? tmpOutRec : tOutHitRecord;
    }

    int prevCellId = -1;
    prevNhits = nHit;
    int	nPrismhited = 0;
    vec3 position = vec3(GLOBLE_RADIUS + GLOBLE_RADIUS);
    vec3 vLightVec = normalize(vec3(6.0f));
    vec4 color;
    vec4 dst = vec4(0.0);

    float boundary = GLOBLE_RADIUS*0.9 + THICKNESS * 3;

    float layerCount = 0.0;


    do{//loop through 3d grid
        if (tInHitRecord.t < 0.0f)
            tInHitRecord.t = 0.0f;//clamp to near plane

        float t = tInHitRecord.t;
        RealType A[12];
        GetAs0(curPrismHitted, A);
        RealType3 fTB[2];
        GetScalarValue(curPrismHitted, fTB);
        RealType OP1, OP4;
        RealType B[8];
        GetBs(curPrismHitted, A, fTB, B, OP1, OP4);
        //compute normal on the three vertices of the top face of current prism.
        RealType3 vtxNormals[3];
        ComputeVerticesNormal(curPrismHitted, vtxNormals);

        while (t <= tOutHitRecord.t && dst.w < 0.98)
        {
            position = ray.o + ray.d * t;
            RealType u, v, t1;
            GetUVT(vec3(0.0f), position, A, u, v, t1);
            float scalar = GetInterpolateValue2(curPrismHitted, u, v, position, fTB[0], fTB[1]);
            color = gcrm_GetColor(scalar);
            if (enableLight)
            {
                Light light = getLightAvg(vec2(u, v), vtxNormals, vLightVec, ray.d);
                color.rgb = color.rgb * light.diffuse + color.a * light.specular;
            }
            dst = (1.0f - dst.w) * color + dst;
            t = t + stepsize;
        }

        //intersect ray with next cell
        if (nextCellId > -1)
        {// more prism to intersect.
            prevCellId = int(curPrismHittedId);
            curPrismHittedId = nextCellId;
            tInHitRecord = tOutHitRecord;
            if (tInHitRecord.nextlayerId <= -1)
            {//shot from inner part to the outside.
                break;
            }
            ReloadVtxInfo(int(curPrismHittedId), tInHitRecord.nextlayerId, curPrismHitted);//update curPrismHitted to nextCell.

            nHit = rayPrismIntersection(curPrismHitted, ray, prevCellId, tInHitRecord, tOutHitRecord, nextCellId);//intersectPrism(ray,prevNhits, tInRec, tOutRec,nextCellId);

            layerCount += 0.1;
        }
        else{
            break;
        }

    } while (nHit > 0 && dst.w < 0.98 && tOutHitRecord.nextlayerId != -1);


    out_Color = dst;

}
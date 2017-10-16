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

#ifndef _GCRM_MESH_H_
#define _GCRM_MESH_H_
#pragma once
#include <unordered_map>
#include <netcdf.h>
#include <map>
#include "IMesh.h"
#include <vec2i.h>
#include <vec3f.h>
#include <GLTexture2D.h>
#include <GLFrameBufferObject.h>
#include <GLVertexBufferObject.h>
#include <GLVertexArray.h>
#include <GLIndexBufferObject.h>
#include <GLPixelBufferObject.h>
#include <GLTextureBufferObject.h>

struct GCRMPrism;
using namespace davinci;
class vec3i;


bool isCCW(davinci::vec3f v0, davinci::vec3f v1, davinci::vec3f v2);

class GCRMMesh :
    public IMesh
{
    //friend class feature_gcrm_t;
public:

    GCRMMesh(void);
    virtual ~GCRMMesh(void);

    virtual void InitShaders(map<string, map<string, string> >& shaderConfig);
    virtual void Refresh();
    virtual void glslSetTransferFunc(const davinci::GLTexture1DRef tfTex);
    //virtual void ReadGrid( const QString& gridFileName,const QStringList& varNames,int time_in, int time_out  );
    virtual void ReadGrid(const string& gridFileName);// , int timeStart, int timeEnd);
    virtual void LoadClimateData(const string& varFileName, const string& varName, int timeStart=0, int timeEnd=1 );

    virtual void Remesh();

    virtual void RenderGrid();

    virtual void ToggleLighting(bool val);
    virtual void ToggleWireframe(bool v);
    virtual void SetLight(davinci::GLLights light);

    virtual int	 GetMaxIdxLayer() const { return m_maxIdxLayer; }
    virtual void SetMaxIdxLayer(int val);
    //virtual void SetSolidMesh(bool val);
    virtual void SetMeshType(const string& meshType);
    virtual void SetStepSize(float val);
    virtual void SetMaterial(davinci::vec4f &material);

    virtual void volumeRender();

protected:
    //normalize the variable 'varName' values
    virtual void normalizeClimateDataArray(const string& varName);

    void CreateVBOTriangleMesh();
    void CreateVBOHexagonMesh();
    void CreateTBOConnectivity();

    void RenderTriangleGridConnc();
    void RenderHexagonalGridConnc();

protected:
    string getUniqueVarName(const string& varFilePath, const string& varName) const;

    int		m_maxIdxLayer;
    nc_type m_dataType;

    //VBO used for geodesic grid geometry definition.
    davinci::GLIndexBufferObjectRef   m_iboTriangle;	//triangle mesh index buffer

    int m_scrWidth, m_scrHeight;
    std::map<string, std::map<string, string> > m_shaderConfig;
    std::unordered_map<std::string, davinci::GLShaderRef> m_shaders;

    davinci::GLVertexBufferObjectRef m_vboSurfaceTriangleVtxPosCellId;//triangle geometry info
    davinci::GLVertexBufferObjectRef m_vboTriangleScalar;//triangle mesh color
    davinci::GLVertexBufferObjectRef m_vboTriangleLayerVtxNmlColor;//triangle mesh layer column lines.
    davinci::GLVertexBufferObjectRef m_vboSurfaceHexagonVtxPosCellId;//hexagon geometry info
    davinci::GLVertexBufferObjectRef m_vboHexagonLayerVtxNmlColor;//Hexagon mesh layer column lines.
    davinci::GLVertexBufferObjectRef m_vboHexagonScalar;//hexagon mesh color

    davinci::GLVertexArrayObjectRef m_pVAOTrianglePosCellIdScalar;//normal triangular grid rendering with colormap.
    davinci::GLVertexArrayObjectRef m_pVAOHexagonGridsVtxCellIdScalar;//hexagon geometry info
    davinci::GLVertexArrayObjectRef m_pVAOHexagonGridsVtxPos;//hexagon geometry for cell id. 
    davinci::GLVertexArrayObjectRef m_pVAOTrianglePos;//triangle geometry for cell id. 
    davinci::GLIndexBufferObjectRef m_iboHexagon;	//hexagon mesh index buffer
  
    //texture1d's for original connectivity information
    davinci::GLTextureBufferObjectRef m_tex1d_grid_center_lat;
    davinci::GLTextureBufferObjectRef m_tex1d_grid_center_lon;
    davinci::GLTextureBufferObjectRef m_tex1d_grid_corner_lat;
    davinci::GLTextureBufferObjectRef m_tex1d_grid_corner_lon;
    davinci::GLTextureBufferObjectRef m_tex1d_grid_center_val;//float
    davinci::GLTextureBufferObjectRef m_tex1d_grid_cell_corners;//int
    davinci::GLTextureBufferObjectRef m_tex1d_grid_edge_corners;//int
    //derived connectivity
    davinci::GLTextureBufferObjectRef m_tex1d_grid_corner_cells;//int
    davinci::GLTextureBufferObjectRef m_tex1d_grid_corner_edges;//int
    davinci::GLTextureBufferObjectRef m_tex1d_grid_edge_cells;//int
    davinci::GLVertexBufferObjectRef m_vbo_triangle_center_id;
    davinci::GLVertexBufferObjectRef m_vbo_hexagon_center_id;
    davinci::GLVertexArrayObjectRef m_vao_triangle_center_id;
    davinci::GLVertexArrayObjectRef m_vao_hexagon_center_id;
    davinci::vec2f m_minmax_lat, m_minmax_lon;
};

typedef std::shared_ptr<GCRMMesh> GCRMMeshRef;
#endif
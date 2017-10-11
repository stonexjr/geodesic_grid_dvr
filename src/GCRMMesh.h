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
class davinci::vec3i;


bool isCCW(davinci::vec3f v0, davinci::vec3f v1, davinci::vec3f v2);

class GCRMMesh :
    public IMesh
{
    //friend class feature_gcrm_t;
public:

    GCRMMesh(void);
    virtual ~GCRMMesh(void);

    virtual davinci::GLShaderRef getGridShader();
    virtual void InitShaders(map<string, map<string, string> >& shaderConfig);
    virtual void Refresh();
    virtual void glslSetTransferFunc(const davinci::GLTexture1DRef tfTex);
    //virtual void ReadGrid( const QString& gridFileName,const QStringList& varNames,int time_in, int time_out  );
    virtual void ReadGrid(const string& gridFileName);// , int timeStart, int timeEnd);
    virtual void LoadClimateData(const string& varFileName, const string& varName, int timeStart=0, int timeEnd=1 );

    virtual void Remesh();

    virtual void RenderGrid(bool bFillMesh=false, bool bLighting=false);



    virtual int	 GetMaxIdxLayer() const { return m_maxIdxLayer; }
    virtual void SetMaxIdxLayer(int val);
    virtual void SetFillMesh(bool val);
    virtual void SetDrawMeshType(const string& meshType);
    virtual void SetStepSize(float val);
    virtual void SetLightParam(davinci::vec4f &lightParam);

    virtual void volumeRender();

protected:
    virtual void CreateVBOTriangleMesh();
    virtual void CreateVBOHexagonMesh();
    virtual davinci::GLVertexBufferObjectRef CreateVBOTriangle();
    virtual davinci::GLIndexBufferObjectRef  CreateIBOHexagon();
    virtual davinci::GLVertexBufferObjectRef CreateVBOTrianglePos();

    virtual  void	RenderRemeshedGridWithLayers(bool bFillMesh, bool bLighting);
    void	RenderHexagonalGridLayers(bool bFillMesh, bool bLighting);

    //normalize the variable 'varName' values
    virtual void normalizeClimateDataArray(const string& varName);

    void RenderTriangleGridConnc(bool bFillMesh, bool bLighting);
    void RenderHexagonalGridConnc(bool bFillMesh, bool bLighting);

protected:
    string getUniqueVarName(const string& varFilePath, const string& varName) const;

    //bool fromGlobalTimeIdToLocalId(int gtimeId, int& iFile, int &localTimeId);
    void CreateTBOConnectivity();
    int		m_maxIdxLayer;
    nc_type m_dataType;
    map<string, davinci::vec2i> m_dataFileTimeStepCounts;

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
    //std::shared_ptr<davinci::GLVertexBufferObject > m_vboHexagonLayerColumn;//triangle mesh layer column lines.
    davinci::GLIndexBufferObjectRef m_iboHexagon;	//hexagon mesh index buffer
  
    //Texture used for worldMap
    davinci::GLTexture2DRef m_tex2d_worldmap;
  
    //render to 3d texture
    davinci::GLVertexArrayObjectRef m_pVAOTrianglePosScalar;//normal triangular grid rendering with colormap.
    davinci::GLVertexBufferObjectRef m_vboTrianglePos;
    davinci::GLFrameBufferObjectRef m_fboResampling;
    davinci::GLTexture3DRef m_tex3d;//resample the original grid to regular grid.
    davinci::GLTexture2DRef m_tex2d;//resample the original grid to regular grid.

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
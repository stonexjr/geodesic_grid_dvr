#ifndef _GCRM_MESH_H_
#define _GCRM_MESH_H_
#pragma once
#include <unordered_map>
#include <netcdf.h>
#include <map>
#include "IMesh.h"
#include <vec2i.h>
#include <GLTexture2D.h>
#include <GLFrameBufferObject.h>
#include <GLVertexBufferObject.h>
#include <GLVertexArray.h>
#include <GLIndexBufferObject.h>
#include <GLPixelBufferObject.h>
#include <GLClickable.h>
#include <GLTextureBufferObject.h>
#include <vec3f.h>

struct GCRMPrism;
class ConfigManager;
class QTFEditor;
class davinci::vec3i;


bool isCCW(davinci::vec3f v0, davinci::vec3f v1, davinci::vec3f v2);

class GCRMMesh :
    public IMesh
{
    friend class feature_gcrm_t;
public:

    GCRMMesh(void);
    virtual ~GCRMMesh(void);

    virtual davinci::GLShaderRef getGridShader();
    virtual void InitShaders();
            void InitTextures();
    virtual void refresh();
    virtual void glslSetTransferFunc(const davinci::GLTexture1DRef tfTex);
    //virtual void ReadGrid( const QString& gridFileName,const QStringList& varNames,int time_in, int time_out  );
    virtual void ReadGrid(const string& gridFileName);// , int timeStart, int timeEnd);
    virtual void LoadClimateData(const string& varFileName, const string& varName, int timeStart=0, int timeEnd=1 );

    virtual void Remesh();

    virtual void RenderGrid(bool bFillMesh=false, bool bLighting=false);

    virtual davinci::GLTextureAbstract* Resampling(int width, int height);
    virtual davinci::GLTextureAbstract* RenderMeshTo3DTexture2(int width, int height);
    //Implement clickable interface
    virtual void resize( int scrWidth, int scrHeight );
    virtual void drawID();
    virtual void drawIDHexgon();
    virtual void drawIDTriangle();
    virtual int  mouseClicked( int x, int y );

    /*
    virtual void RenderVolumeCPU( uint* d_output, uint* d_input, int m_scrWidth, int m_scrHeight );
    virtual void RenderVolumeGPU( uint* d_output, uint* d_inCellId, const float3* d_pixelPosition,
                                   int m_scrWidth, int m_scrHeight, cudaStream_t stream=0 );
    */


    virtual davinci::GLFrameBufferObjectRef getFBOVizSurfaceRef(){ return m_fboVisSurface;};
    /*
    virtual void uploadCellGrids();
    virtual void uploadClimateValue();
    virtual void updateHistogram(QTFEditor* tfEditor);

    */
    virtual int	 GetMaxIdxLayer() const { return m_maxIdxLayer; }
    virtual void SetMaxIdxLayer(int val);
    virtual void setInVisibleCellId(int m_maxNcells);
    virtual void SetFillMesh(bool val);
    virtual void SetDrawMeshType(const string& meshType);
    virtual void SetTiming(float timing);
    virtual void SetStepSize(float val);
    virtual void SetLightParam(davinci::vec4f &lightParam);
    virtual void SetSplitRange(davinci::vec2f range);

    void	SetWorldMapImagePath( const QVector<QString>& worldMapImagePaths );

    //int     GetVariableTimeSliceCount(const QString& varFileName, const QString& varName);

    //Return a mangled climate variable name by prefixing the file name.
    virtual QString GetClimateVariableName() const;
    //Specify the time slice id to which the data you need to render.

    //return if timeId is within global time steps range.
    virtual bool SetCurrentGlobalTimeSliceId(int timeId);
    virtual void volumeRender();
protected:
    virtual void CreateVBOTriangleMesh();
    virtual void CreateVBOHexagonMesh();
    virtual davinci::GLVertexBufferObjectRef CreateVBOTriangle();
    virtual davinci::GLIndexBufferObjectRef  CreateIBOHexagon();
    virtual davinci::GLVertexBufferObjectRef CreateVBOTrianglePos();

    virtual  void	RenderRemeshedGridWithLayers(bool bFillMesh, bool bLighting);
    void	RenderHexagonalGridLayers(bool bFillMesh, bool bLighting);

    /*
    float   GetValue(float3& pos, const GCRMPrism& prism);
    float	GetValueTest(float3& pos);
    float3  GetNormal(float3& pos);
    float2  sphericalMapping( const float3 &position );
    void	HijackClimateData( HostFloatArrayRef & _h_climateValsRef );
inline float computeScalarCenterToPole(const int iCorner,const int iLayer,const float delLayer,
                            const float sin_iLat,const float sin_iLon,const float cos_iLat,const float cos_iLon);
inline float computeScalar2(const int iCorner,const int iLayer,const float delLayer, const float theta,
                            const float sin_iLat,const float sin_iLon,const float cos_iLat,const float cos_iLon);

    virtual void bindVisSurfacePixPositionFBO();
    virtual void unbindVisSurfacePixPositionFBO();
    virtual void bindVisibleSurfaceTexture();
    virtual void unbindVisibleSurfaceTexture();

    virtual GLuint GetCellIdTexId();

    virtual GLuint GetPixelPositionTexId();
    */
    //////////////////////////////////////////////////////////////////////////
    //virtual void cuSetClipeRect( const RectInt2D& m_clipeRect );
    //normalize the variable 'varName' values
    virtual void normalizeClimateDataArray(const string& varName);

    void RenderTriangleGridConnc(bool bFillMesh, bool bLighting);
    void RenderHexagonalGridConnc(bool bFillMesh, bool bLighting);

protected:
    string getUniqueVarName(const string& varFilePath, const string& varName) const;

    bool fromGlobalTimeIdToLocalId(int gtimeId, int& iFile, int &localTimeId);
    void CreateTBOConnectivity();
    int		m_maxIdxLayer;
    nc_type m_dataType;
    map<string, davinci::vec2i> m_dataFileTimeStepCounts;
    //attachment0:texture attached to FBO, the world coordinates of entry point 
    // of the sphere, which match one pixel, is rendered as color component in the texture.
    davinci::GLFrameBufferObjectRef m_fboVisSurface;
    davinci::GLTexture2DRef m_texPixelPos;
    //attachment1:texture attached to FBO, the id of the visible cell of the sphere
    //is rendered as color component in the texture.
    davinci::GLTexture2DRef m_texTriaCellId;

    //glReadPixel() read texture content into CPU m_PositionBuffer for CPU volume rendering.
    std::vector<float> m_fragPos;

    //VBO used for geodesic grid geometry definition.
    davinci::GLIndexBufferObjectRef   m_iboTriangle;	//triangle mesh index buffer

    int m_scrWidth, m_scrHeight;
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
    /*
    davinci::GLPixelBufferObjectRef m_cudaGLResourceWorldMap[2][2];
    //////GPU1 data array//////////
    DevFloatArrayRef m_pDevTriangle_corner_lat1;
    DevFloatArrayRef m_pDevTriangle_corner_lon1;
    DevIntArrayRef   m_pDevCorner_edges1;
    DevIntArrayRef   m_pDevCorner_cells1;
    DevIntArrayRef   m_pDevEdge_cells1;
    DevIntArrayRef   m_pDevEdge_corners1;
    DevIntArrayRef   m_pDevCell_corners1;
    DevFloatArrayRef m_pDevClimateVals1;
    //////GPU2 data array//////////
    DevFloatArrayRef m_pDevTriangle_corner_lat2;
    DevFloatArrayRef m_pDevTriangle_corner_lon2;
    DevIntArrayRef   m_pDevCorner_edges2;
    DevIntArrayRef   m_pDevCorner_cells2;
    DevIntArrayRef   m_pDevEdge_cells2	;
    DevIntArrayRef   m_pDevEdge_corners2;
    DevIntArrayRef   m_pDevCell_corners2;
    DevFloatArrayRef m_pDevClimateVals2	;
    */
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
    bool m_bDoneResampling;

};

typedef std::shared_ptr<GCRMMesh> GCRMMeshRef;
#endif
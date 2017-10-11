#ifndef _IMESH_H_
#define _IMESH_H_
#include <map>
#include "vec3f.h"
//#include "config.h"
#include "HostArrayTemplate.h"
#include <GLFrameBufferObject.h>
#include <GLTexture1D.h>
#include <GLTexture3D.h>
#include <GLShader.h>

class NetCDFFile;
class NetCDFManager;
typedef std::shared_ptr<NetCDFFile> NetCDFFileRef;
typedef std::shared_ptr<NetCDFManager> NetCDFManagerRef ;

class IMesh
{
public:
    IMesh();
    IMesh(const NetCDFManagerRef& netmngr);

    virtual ~IMesh(void);

    virtual davinci::GLShaderRef getGridShader()=0;
    virtual void InitShaders(map<string, map<string, string> >& shaderConfig)=0;
    virtual void Refresh()=0;
    //virtual void InitTexture();
    //virtual void updateHistogram(QTFEditor * tfEditor)=0; 
    //set transfer function for GLSL program. 
    virtual void glslSetTransferFunc(const davinci::GLTexture1DRef tfTex){m_tfTex = tfTex;};
    //Should be called when screen resizes.
    //virtual void Resize(int scrWidth, int scrHeight)=0;
    //for GL_RGBA GL_UNSIGNED_BYTE
    /*
    virtual void RenderVolumeCPU( uint* d_output, uint* d_input, int m_scrWidth, int m_scrHeight )=0;
    //for GL_RGBA GL_UNSIGNED_BYTE
    virtual void RenderVolumeGPU( uint* d_output, uint* d_inCellId, const float3* d_pixelPosition,
                                   int m_scrWidth, int m_scrHeight, cudaStream_t stream=0 )=0;
    */
    //fileName: file name with full path.
    //load grid file and return the reference of the newly loaded file
    //return NULL if failed on loading.
    virtual void ReadGrid(const string& gridFilePath)=0;
            void LoadGridVar(const string& gridFileName,const vector<string>& varNames);
    virtual void Remesh()=0;
    virtual void RenderGrid(bool bFillMesh=false, bool bLighting=false)=0;
    virtual davinci::GLTextureAbstract* RenderMeshTo3DTexture(int width, int height);
    //fileName: file name with full path.
    virtual void LoadClimateData(const string& fileName, const string& varName, int timeStart=0, int timeEnd=1)=0;
    virtual void normalizeClimateDataArray(const string& varName)=0;
    //virtual void uploadCellGrids()=0;
    //virtual void uploadClimateValue()=0;
    virtual int	 GetMaxIdxLayer() const = 0;
    virtual void SetMaxIdxLayer(int val) = 0;
    string GetGridFileName() const { return m_gridSimpleFileName; }
    virtual string GetClimateVariableName() const { return m_climateVariableUniqueId; }
//    QString GetClimateVaribleFileName() const { return m_climateVaribleFileName; }
//    void	SetClimateVaribleFileName(const QString& val) { m_climateVaribleFileName = val; }
    /*
    virtual bool SetCurrentGlobalTimeSliceId(int timeId){ m_timeGlobal = timeId; return true;}
    int     GetCurrentGlobalTimeSliceId() const { return  m_timeGlobal;}
    int     GetCurrentLocalTimeSliceId() const { return  m_curLocalTimeStep;}
    */
    int     GetTotalTimeSteps() const { return  m_totalTimeSteps;}
    int     GetCurrentFileId() const { return  m_curFileStep;}

    HostIntArrayRef   GetIntArray(const string& varName);
    HostFloatArrayRef GetFloatArray(const string& varName);
    HostDoubleArrayRef GetDoubleArray(const string& varName);
    bool IsFillMesh() const { return m_bFillMesh; }
    virtual void SetFillMesh(bool val) { m_bFillMesh = val; }

    virtual void bindVisSurfacePixPositionFBO();
    virtual void unbindVisSurfacePixPositionFBO();
    virtual void bindVisibleSurfaceTexture();
    virtual void unbindVisibleSurfaceTexture();

    virtual void SetDrawMeshType(const string& meshType ){ m_meshType = meshType;};
    string GetDrawMeshType() const { return m_meshType;}

    //GridType GetGridType() const { return m_gridType;}

    davinci::vec3f   GetBGColor3f() const { return m_bgColor3f; }
    void    SetBGColor3f(const davinci::vec3f& val) { m_bgColor3f = val; }
    //If the data has been reloaded, set it true before rendering of next frame.
    //So that the VBO color buffer will updated accordingly.
    void        SetDataNew(bool isNew){ m_isDataNew = isNew;}
    //If the data has been reloaded upon rendering of next frame.
    bool        GetIsDataNew() const { return m_isDataNew;}

    virtual void setInVisibleCellId(int m_maxNcells);
    virtual void SetTiming(float timing);
    virtual void SetStepSize(float val);
    virtual void SetLightParam(davinci::vec4f &lightParam);
    virtual void SetSplitRange(davinci::vec2f range);
    davinci::vec2f GetSplitRange()const{ return m_split_range; }
    virtual void volumeRender();

protected:

    NetCDFFileRef createGridFile(const string& gridFilePath);

    //QString makeSimpleFileName( const QString& fileName ) const;
    /*
    virtual uint GetCellIdTexId();
    virtual uint GetPixelPositionTexId();
    */
    //virtual void cuSetClipeRect( const RectInt2D& m_clipeRect );
protected:

    float m_timing;
    int m_timeGlobal;
    int m_curFileStep, m_curLocalTimeStep;
    int m_totalTimeSteps;
    davinci::vec3f  m_bgColor3f;
    static char* m_interpName[3];
    string m_meshType;
    NetCDFManagerRef m_netmngr;
    string m_gridSimpleFileName;//eg. "grid.nc"
    string m_climateVariableFileName;//eg. "vorticity_19010101_000000.nc"
    string m_climateVariableUniqueId;//eg. "vorticity_19010101_000000.nc_vorticity"
    string m_climateVariableName;//eg. "vorticity"
    string m_datasetPath;//eg."H:/fusion/data/atmos/atmos_220km/gcrm_220km/"
    bool    m_bFillMesh;
    bool    m_isDataNew;
    davinci::GLTexture1DRef m_tfTex;
    davinci::vec2f m_split_range;
};
typedef std::shared_ptr<IMesh> IMeshRef;
#endif
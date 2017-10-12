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
#include <GLContext.h>

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

    virtual void InitShaders(map<string, map<string, string> >& shaderConfig)=0;
    virtual void Refresh()=0;
    //set transfer function for GLSL program. 
    virtual void glslSetTransferFunc(const davinci::GLTexture1DRef tfTex){m_tfTex = tfTex;};
   
    //fileName: file name with full path.
    //load grid file and return the reference of the newly loaded file
    //return NULL if failed on loading.
    virtual void ReadGrid(const string& gridFilePath)=0;
            void LoadGridVar(const string& gridFileName,const vector<string>& varNames);
    virtual void Remesh()=0;
    virtual void RenderGrid()=0;
    //fileName: file name with full path.
    virtual void LoadClimateData(const string& fileName, const string& varName, int timeStart=0, int timeEnd=1)=0;
    virtual void normalizeClimateDataArray(const string& varName)=0;
    virtual int	 GetMaxIdxLayer() const = 0;
    virtual void SetMaxIdxLayer(int v) = 0;
    virtual void ToggleLighting(bool v) { m_bLighting = v; };
    virtual void ToggleWireframe(bool v) { m_bWireframe = v; };
    virtual void SetLight(davinci::GLLights light)=0;

    string GetGridFileName() const { return m_gridSimpleFileName; }
    virtual string GetClimateVariableName() const { return m_climateVariableUniqueId; }
    int     GetTotalTimeSteps() const { return  m_totalTimeSteps;}
    int     GetCurrentFileId() const { return  m_curFileStep;}

    HostIntArrayRef   GetIntArray(const string& varName);
    HostFloatArrayRef GetFloatArray(const string& varName);
    HostDoubleArrayRef GetDoubleArray(const string& varName);

    virtual void bindVisSurfacePixPositionFBO();
    virtual void unbindVisSurfacePixPositionFBO();
    virtual void bindVisibleSurfaceTexture();
    virtual void unbindVisibleSurfaceTexture();

    virtual void SetMeshType(const string& meshType ){ m_meshType = meshType;};
    string GetDrawMeshType() const { return m_meshType;}

    davinci::vec3f   GetBGColor3f() const { return m_bgColor3f; }
    void    SetBGColor3f(const davinci::vec3f& val) { m_bgColor3f = val; }
    //If the data has been reloaded, set it true before rendering of next frame.
    //So that the VBO color buffer will updated accordingly.
    void        SetDataNew(bool isNew){ m_isDataNew = isNew;}
    //If the data has been reloaded upon rendering of next frame.
    bool        GetIsDataNew() const { return m_isDataNew;}

    virtual void SetStepSize(float val)=0;
    virtual void SetMaterial(davinci::vec4f &material)=0;
    virtual void volumeRender()=0;

protected:

    NetCDFFileRef createGridFile(const string& gridFilePath);

    //QString makeSimpleFileName( const QString& fileName ) const;
    /*
    virtual uint GetCellIdTexId();
    virtual uint GetPixelPositionTexId();
    */
    //virtual void cuSetClipeRect( const RectInt2D& m_clipeRect );
protected:

    bool m_bLighting;
    bool m_bWireframe;
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
    bool    m_isDataNew;
    davinci::GLTexture1DRef m_tfTex;
};
typedef std::shared_ptr<IMesh> IMeshRef;
#endif
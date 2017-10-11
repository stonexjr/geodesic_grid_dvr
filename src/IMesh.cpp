#include "IMesh.h"
#include "GLTexture1D.h"
#include "GLError.h"
#include "DError.h"
#include "NetCDFFile.h"
#include "NetCDFManager.h"
#include "Dir.h"

char* IMesh::m_interpName[3]={"Prism", "Tetra", "MVI"};

IMesh::IMesh(void)
    :m_netmngr(new NetCDFManager()),m_gridSimpleFileName(""),
    m_bFillMesh(false),m_isDataNew(true),
    m_timeGlobal(0),m_curFileStep(0),m_curLocalTimeStep(0),
    m_totalTimeSteps(0), m_timing(0.0f)
{
}

IMesh::IMesh( const NetCDFManagerRef& netmngr )
    :m_netmngr(netmngr)
{
}

IMesh::~IMesh(void)
{
}

NetCDFFileRef IMesh::createGridFile( const string& gridFilePath )
{
    ifstream ifs(gridFilePath, ios::binary);
    if (!ifs){
        davinci::GLError::ErrorMessage(Dir::basename(gridFilePath) + " not found.\n Please specify a correct file path");
        exit(1);
    }
    m_netmngr->addNetCDF(gridFilePath);
    m_gridSimpleFileName = Dir::basename(gridFilePath);

    //print out dimensional variable names for sanity check;
    int nVariables;
    int dim = 2;
    vector<string> varNames = (*m_netmngr)[m_gridSimpleFileName]->GetVarNames(&nVariables, dim);

    cout <<"===========var names with dimension="<<dim<<"==============\n";
    std::for_each(varNames.begin(), varNames.end(), [](string& name){ cout << name << endl; });

    return (*m_netmngr)[m_gridSimpleFileName];
}

HostIntArrayRef IMesh::GetIntArray( const string& varName )
{
    //for each netcdf file managed by netcdf file manager
    HostIntArrayRef pArray =
        (*m_netmngr)[m_gridSimpleFileName]->getIntArray()[varName];
    if (pArray)
    {
        return pArray;
    }

    return (*m_netmngr)[m_climateVariableFileName]->getIntArray()[varName];
}

HostFloatArrayRef IMesh::GetFloatArray( const string& varName )
{
    HostFloatArrayRef pArray =
        (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()[varName];
    if (pArray)
    {
        return pArray;
    }

    return (*m_netmngr)[m_climateVariableFileName]->getFloatArray()[varName];
}

HostDoubleArrayRef IMesh::GetDoubleArray( const string& varName )
{
    HostDoubleArrayRef pArray =
        (*m_netmngr)[m_gridSimpleFileName]->getDoubleArray()[varName];
    if (pArray)
    {
        return pArray;
    }

    return (*m_netmngr)[m_climateVariableFileName]->getDoubleArray()[varName];
}

void IMesh::bindVisSurfacePixPositionFBO()
{
    D_ABORT("The method or operation is not implemented.", DERROR_ERR_FUN_INVALID);
}

void IMesh::unbindVisSurfacePixPositionFBO()
{
    D_ABORT("The method or operation is not implemented.", DERROR_ERR_FUN_INVALID);
}

void IMesh::bindVisibleSurfaceTexture()
{
    D_ABORT("The method or operation is not implemented.", DERROR_ERR_FUN_INVALID);
}

void IMesh::unbindVisibleSurfaceTexture()
{
    D_ABORT("The method or operation is not implemented.", DERROR_ERR_FUN_INVALID);
}

void IMesh::LoadGridVar(const string& gridFileName,const vector<string>& varNames)//, int time_in, int time_out )
{
    if (!m_netmngr)
    {
        davinci::GLError::ErrorMessage(string("GCRMMesh::ReadGrid(): netcdf manager is null"));
    }
    string gridSimpleName = Dir::basename(gridFileName);
    NetCDFFileRef netcdf = (*m_netmngr)[gridSimpleName];
    if (!netcdf)
    {
        netcdf = createGridFile(gridFileName);
        if (!netcdf)
        {
            davinci::GLError::ErrorMessage(gridFileName + string(" not exist!"));
        }
    }

    //The variable data array is loaded from here.
    std::for_each(varNames.begin(), varNames.end(), [&](const string& varName){
        NetCDF_var* pVarInfo = netcdf->GetVarInfo(varName);
        if (!pVarInfo)
        {
            davinci::GLError::ErrorMessage(string(__func__)+string(": cannot find variable ")+varName);
        }
        switch (pVarInfo->rh_type)
        {
            case NC_INT:
                netcdf->addIntArray(varName, varName);
                break;
            case NC_FLOAT:
                netcdf->addFloatArray(varName, varName);
                break;
            case NC_DOUBLE:
                netcdf->addDoubleArray(varName, varName);
        }
        netcdf->LoadVarData(varName, varName, 0, 1, 0, 1, true);
    });
}


davinci::GLTextureAbstract* IMesh::RenderMeshTo3DTexture( int width, int height )
{
    throw std::exception("IMesh::RenderMeshTo3DTexture() not yet implemented!");
}

void IMesh::setInVisibleCellId(int m_maxNcells)
{
    throw std::logic_error("The method or operation is not implemented.");
}

void IMesh::SetTiming(float timing)
{
    throw std::logic_error("The method or operation is not implemented.");
}

void IMesh::volumeRender()
{
    throw std::logic_error("The method or operation is not implemented.");
}

void IMesh::SetStepSize(float val)
{
    throw std::exception("The method or operation is not implemented.");
}

void IMesh::SetLightParam(davinci::vec4f &lightParam) 
{
    throw std::logic_error("The method or operation is not implemented.");
}

void IMesh::SetSplitRange(davinci::vec2f range)
{
    throw std::logic_error("The method or operation is not implemented.");
}

/*
uint IMesh::GetCellIdTexId()
{
    D_ABORT("The method or operation is not implemented.", DERROR_ERR_FUN_INVALID);
}

uint IMesh::GetPixelPositionTexId()
{
    D_ABORT("The method or operation is not implemented.", DERROR_ERR_FUN_INVALID);
}
*/

/*
void IMesh::cuSetClipeRect( const RectInt2D& m_clipeRect )
{
    D_ABORT("The method or operation is not implemented.", DERROR_ERR_FUN_INVALID);
}
*/


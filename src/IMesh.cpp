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
    m_bWireframe(true),m_isDataNew(true),
    m_timeGlobal(0),m_curFileStep(0),m_curLocalTimeStep(0),
    m_totalTimeSteps(0), m_bLighting(true)
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

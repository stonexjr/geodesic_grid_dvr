#include "GCRMMesh.h"
#include "GL/glew.h"
//#include "GL/glut.h"
#include <chrono>
#include <numeric>
#include <map>
//#include <QGLWidget>
#include <vec3i.h>
//#include "util.h"
//#include "ConfigManager.h"
#include "GLError.h"
#include <GLUtilities.h>
//#include "QTFEditor.h"
#include "GLError.h"
#include "GLContext.h"
#include "NetCDFFile.h"
#include "NetCDFManager.h"
#include "Dir.h"
#include "HostArrayTemplate.h"

#define MAIN_GPU_ID 0
using namespace davinci;

struct Vec3f_INT{
    davinci::vec3f position;
    int  cellId;
};

GCRMMesh::GCRMMesh(void):
    IMesh(),m_maxIdxLayer(0)
{
    //m_gridType = GCRMMESH;
    SetDrawMeshType("triangle");
    davinci::GLError::glCheckError(__func__);
    m_split_range = vec2f(0.0);
}

GCRMMesh::~GCRMMesh(void)
{
} 

GLShaderRef GCRMMesh::getGridShader()
{
    return m_shaders["grid_gcrm"];//m_volumeRender->GetShader("grid_gcrm");
}

void GCRMMesh::InitShaders(map<string, map<string, string> >& shaderConfig)
{
    m_shaderConfig = shaderConfig;
    vec4f lightParam(0.5f/*Kamb*/, 0.83f/*Kdif*/, 0.89f/*Kspe*/, 20.0f/*Kshi*/);
    cout << "** lightParam=" << lightParam << endl;
    //shader for rendering mesh directly from connectivity texture
    //dual triangular mesh
    string shaderName = "gcrm_triangle_mesh";
    GLShaderRef
    shader = GLShaderRef(new GLShader(shaderName, true));
    shader->loadVertexShaderFile(shaderConfig[shaderName]["vertex_shader"]);// qPrintable(config->m_shaderFiles[3]));
    shader->loadGeomShaderFile(shaderConfig[shaderName]["geometry_shader"]);// qPrintable(config->m_shaderFiles[4]));
    shader->loadFragShaderFile(shaderConfig[shaderName]["fragment_shader"]);// qPrintable(config->m_shaderFiles[5]));
    shader->CreateShaders();
    shader->SetIntUniform("enableLight", true);// config->m_bEnableLighting);
    m_shaders[shader->getShaderName()] = shader; 

    //hexagon mesh 
    shaderName = "gcrm_hexagon_mesh";
    shader = GLShaderRef(new GLShader("gcrm_hexagon_mesh", true));
    shader->loadVertexShaderFile(shaderConfig[shaderName]["vertex_shader"]);// qPrintable(config->m_shaderFiles[6]));
    /*
    */
    if (m_bFillMesh)
    {
        shader->loadGeomShaderFile(shaderConfig[shaderName]["geometry_shader_fill"]);// qPrintable(config->m_shaderFiles[8]));//solid fill
    }else{
        shader->loadGeomShaderFile(shaderConfig[shaderName]["geometry_shader_frame"]);// qPrintable(config->m_shaderFiles[7]));//frame
    }
    //  shader->loadGeomShaderFile(qPrintable(config->m_shaderFiles[7]));//frame
    shader->loadFragShaderFile(shaderConfig[shaderName]["fragment_shader"]);// qPrintable(config->m_shaderFiles[9]));
    shader->CreateShaders();
    shader->SetIntUniform("enableLight", true);
    m_shaders[shader->getShaderName()] = shader;

    //volume ray casting using my proposed method
    shaderName = "gcrm_dvr";
    shader = GLShaderRef(new GLShader("gcrm_dvr", true));
    shader->loadVertexShaderFile(shaderConfig[shaderName]["vertex_shader"]);// qPrintable(config->m_shaderFiles[3]));//TODO
    shader->loadGeomShaderFile(shaderConfig[shaderName]["geometry_shader"]);// qPrintable(config->m_shaderFiles[10]));
    shader->loadFragShaderFile(shaderConfig[shaderName]["fragment_shader"]);// qPrintable(config->m_shaderFiles[11]));
    shader->CreateShaders();
    shader->SetIntUniform("enableLight", true);
    shader->SetFloatUniform("stepsize", 0.01f);
    shader->SetFloat4Uniform("lightParam", lightParam);
    m_shaders[shader->getShaderName()] = shader; 
}

void GCRMMesh::Refresh()
{
    for (std::pair<const std::string, GLShaderRef>& p: m_shaders)
    {
        p.second->refresh();
    }
}

void GCRMMesh::SetFillMesh(bool val)
{
    m_bFillMesh = val;
    if (m_meshType == "hexagon")
    {
        string shaderName = "gcrm_hexagon_mesh";
        GLShaderRef shader = m_shaders[shaderName];
        if (m_bFillMesh)
        {
            shader->loadGeomShaderFile(m_shaderConfig[shaderName]["geometry_shader_fill"]);// qPrintable(config->m_shaderFiles[8]));//solid fill
        }else{
            shader->loadGeomShaderFile(m_shaderConfig[shaderName]["geometry_shader_frame"]);// qPrintable(config->m_shaderFiles[7]));//frame
        }
        shader->CreateShaders();
    }
}

void GCRMMesh::SetDrawMeshType(const string& meshType)
{
    m_meshType = meshType;
    if (m_meshType == "hexagon" && !m_shaders.empty())
    {
        SetFillMesh(m_bFillMesh);
    }
}

void GCRMMesh::glslSetTransferFunc( const davinci::GLTexture1DRef tfTex )
{
    IMesh::glslSetTransferFunc(tfTex);
    //IMesh::InitShaders();
    /*
    GLShaderRef shader = m_shaders["grid_gcrm"];//m_volumeRender->GetShader("grid_gcrm");
    if (!shader)
    {
        GLError::ErrorMessage(string(__func__)+string("Please initialize gridShader by calling GCRMMesh::InitShaders()!\n"));
    }
    shader->SetSamplerUniform("tf", &*m_tfTex);
    */

    GLShaderRef
    shader = m_shaders["gcrm_triangle_mesh"];
    shader->SetSamplerUniform("tf", &*m_tfTex);

    shader = m_shaders["gcrm_hexagon_mesh"];
    shader->SetSamplerUniform("tf", &*m_tfTex);
    
    shader = m_shaders["gcrm_dvr"];
    shader->SetSamplerUniform("tf", &*m_tfTex);
}

//***********************************************************************************
// Access:    virtual public
// Description: Remesh the GCRM mesh by generating corner_to_cells and corner_to_edges
// table.
//***********************************************************************************
void GCRMMesh::Remesh()
{
    HostIntArrayRef varCellCorners =
        (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["cell_corners"];
    HostFloatArrayRef varCornerLat =
        (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lat"];
    HostFloatArrayRef varCornerLon =
        (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lon"];

    if(! varCellCorners || !varCornerLat || !varCornerLon){
        cerr << (string("ReMesh() Error: one of cell_corners, grid_corner_lat or grid_corner_lon is NULL.\n"));
    }
    int nCorners = (int)varCornerLat->m_dimsizes[0];
    //int nCorners2 = varCornerLat->m_vars.size();
    HostIntArrayRef varCornerCells =
        HostIntArrayRef(new HostIntArrayType(HostIntArrayType::DIM2, nCorners,3)); 
    varCornerCells->fill(-1);//initialize with invalid index value.

    int nCell = (int)varCellCorners->m_dimsizes[0];
    int nCornerPerCell = (int)varCellCorners->m_dimsizes[1];
    int n=0;
    //Generate corner_to_hexCell table.
    for(int iCell=0; iCell < nCell ; ++iCell)
    {//for each cell
        for(int i=0; i < nCornerPerCell ; ++i)
        {//for each corner of a cell
            int  vtxIdx	= (*varCellCorners)[iCell*nCornerPerCell+i];
            int  j=0;
            bool bHasDuplicate = false;
            while (j<3 && (*varCornerCells)[vtxIdx*3+j] != -1 ){//find the nearest slot with invalid value
                if ((*varCornerCells)[vtxIdx*3+j]==iCell){
                    //Degeneracy: hexagon cell becomes pentagon.
                    //avoid duplicate corner. Because there're some pentagons mixed in the hexagon mesh.
                    bHasDuplicate = true;
                    break;
                }

                ++j;
            }
            if (j<3 && !bHasDuplicate)
            {
                (*varCornerCells)[vtxIdx*3+j] = iCell;
            }
        }
    }
    (*m_netmngr)[m_gridSimpleFileName]->insertIntArray("corner_cells", varCornerCells);
    //Generate corner_to_hexEdges table. Each hexagon corner is shared by three edges.
    HostIntArrayRef varEdgeCorners = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["edge_corners"];
    if(! varEdgeCorners){
        GLError::ErrorMessage(string("Remesh() Error: edge_corners array is NULL.\n"));
    }
    int nCornerPerEdge = (int) varEdgeCorners->m_dimsizes[1];//
    assert(nCornerPerEdge == 2);
    int nEdgesPerCorner = 3;
    int nEdges = (int) varEdgeCorners->m_dimsizes[0];
    HostIntArrayRef varCornerEdges = HostIntArrayRef(new HostIntArrayType(HostIntArrayType::DIM2, nCorners, nEdgesPerCorner));
    varCornerEdges->fill(-1);
    for (int iEdge = 0; iEdge < nEdges; ++iEdge){
        //for each edge
        for (int i = 0 ; i < 2 ; ++i){
            //for each end point of the edge
            int vtxIdx = (*varEdgeCorners)[iEdge*nCornerPerEdge + i];
            int j=0;
            while(j<3 && (*varCornerEdges)[vtxIdx*nEdgesPerCorner+j] != -1){//find the nearest empty slot(-1) for the vtxIdxth entry
                ++j;
            }
            if (j < 3){
                (*varCornerEdges)[vtxIdx*nEdgesPerCorner + j] = iEdge;
            }
        }
    }
    (*m_netmngr)[m_gridSimpleFileName]->insertIntArray("corner_edges",varCornerEdges);
    //Generate edge_to_hexCells table. Each hexagon corner is shared by three edges.
    HostIntArrayRef varCellEdges = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["cell_edges"];
    if(! varCellEdges){
        GLError::ErrorMessage(string("Remesh() Error: cell_edges array is NULL.\n"));
    }
    int nEdgesPerCell = (int) varCellEdges->m_dimsizes[1];
    assert(nEdgesPerCell == 6);
    int nCellsPerEdge = 2;
    HostIntArrayRef varEdgeCells = HostIntArrayRef(new HostIntArrayType(HostIntArrayType::DIM2, nEdges, nCellsPerEdge) );
    varEdgeCells->fill(-1);
    for (int iCell = 0; iCell < nCell ; iCell++){//for each hex cell
        //int idxPrevEdge = -1;
        for (int iEdge = 0; iEdge < 6 ; iEdge++){//for each hex edge
            int  idxEdge = (*varCellEdges)[iCell * 6 + iEdge];//global index of ith edge of iCell.
            int  j=0;
            bool bHasDuplicate = false;
            while(j < 3 && (*varEdgeCells)[idxEdge * 2 + j] != -1){
                //find the nearest empty slot(-1) for the idxEdge'th entry
                if ((*varEdgeCells)[idxEdge * 2 + j] == iCell){
                    //Degeneracy: hexagon cell becomes pentagon.
                    //avoid duplicate edge. Because there're some pentagons mixed in the hexagon mesh.
                    bHasDuplicate = true;
                    //qDebug("Duplicated edge found in cell%d, %dth edge(idx=%d) and %dth edge(idx=%d)\n",
                    //	iCell, iEdge, idxEdge, iEdge-1, idxPrevEdge );
                    break;
                }
                ++j;
            }
            if (j < 3  && !bHasDuplicate){
                (*varEdgeCells)[idxEdge * 2 + j] = iCell;
            }
            //idxPrevEdge = idxEdge;
        }
    }
    (*m_netmngr)[m_gridSimpleFileName]->insertIntArray("edge_cells",varEdgeCells);
}


//#define DEBUG_RESAMPLING

void GCRMMesh::RenderGrid( bool bFillMesh, bool bLighting/*=false*/ )
{
    if (m_meshType == "triangle")
    {
        //RenderRemeshedGridWithLayers(bFillMesh, bLighting);
        //printf("**********RenderRemeshedGrid**********\n.");
        //RenderRemeshedGrid(bFillMesh, bLighting);
        RenderTriangleGridConnc(bFillMesh, bLighting);
        //Resampling(2048, 2048);
#ifndef DEBUG_RESAMPLING
        //Resampling(1024, 1024);
        //Resampling(200, 100);
#else
        Resampling(m_scrWidth, m_scrHeight);
#endif
        //printf("**********end of RenderRemeshedGrid**********\n.");
    }
    else
    {
        //RenderHexagonalGridLayers(bFillMesh, bLighting);
        //printf("********hexagon*******\n");
        //RenderHexagonalGrid(bFillMesh, bLighting);
        RenderHexagonalGridConnc(bFillMesh, bLighting);
    }
}

void GCRMMesh::CreateTBOConnectivity()
{
    NetCDFFileRef gridFile = (*m_netmngr)[m_gridSimpleFileName];
    NetCDFFileRef climateVariableFile = (*m_netmngr)[m_climateVariableFileName];

    if (!gridFile || !climateVariableFile)
    {
        GLError::ErrorMessage(string(__func__) + m_climateVariableFileName + "is not load!");
    }
    HostFloatArrayRef	varCenterLat = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lat"];
    HostFloatArrayRef	varCenterLon = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lon"];
    HostFloatArrayRef	varCornerLat = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lat"];
    HostFloatArrayRef	varCornerLon = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lon"];
    HostFloatArrayRef	varClimate = climateVariableFile->getFloatArray()[m_climateVariableUniqueId];
    HostIntArrayRef   varCellCorners = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["cell_corners"];
    //derived connectivity
    HostIntArrayRef	  varCornerCells = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["corner_cells"];
    HostIntArrayRef    varCornerEdges = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["corner_edges"];
    HostIntArrayRef    varEdgeCells = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["edge_cells"];
    HostIntArrayRef    varEdgeCorners = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["edge_corners"];

    if (!varClimate || !varCenterLat || !varCenterLat || !varCenterLon || !varCellCorners
        || !varCornerCells || !varCornerEdges || !varEdgeCells || !varEdgeCorners //	|| !varInterfaces || !varLayers
        )
    {
        GLError::ErrorMessage(string(__func__) + string(": varCornerCells, varCenterLat, varCenterLon , varNEdgesOnCells or varClimate is NULL!"));
    }

    GLShaderRef shader_triangle = m_shaders["gcrm_triangle_mesh"];
    GLShaderRef shader_hexagon  = m_shaders["gcrm_hexagon_mesh"];
    GLShaderRef shader_dvr      = m_shaders["gcrm_dvr"];
    GLShaderRef shader_dvr_regular = m_shaders["gcrm_dvr_regular"];
    GLShaderRef shader_resample = m_shaders["resampling"];

    assert(shader_triangle);
    assert(shader_hexagon);
    assert(shader_resample);
    assert(shader_dvr);
    assert(shader_dvr_regular);
    
    if (!m_tex1d_grid_edge_corners)
    {
        int nEdges   = (int) varEdgeCorners->m_dimsizes[0];
        int nDegrees = (int) varEdgeCorners->m_dimsizes[1];
        assert(nDegrees == 2);
        m_tex1d_grid_edge_corners = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_RG32I, GL_STATIC_DRAW));
        m_tex1d_grid_edge_corners->setName("edge_corners");
        m_tex1d_grid_edge_corners->upload(nEdges*nDegrees*sizeof(int), nEdges, varEdgeCorners->data());
    }
    shader_dvr->SetSamplerUniform("edge_corners", &*m_tex1d_grid_edge_corners);

    if (!m_tex1d_grid_center_lat)
    {
        int nCenters = (int) varCenterLat->m_dimsizes[0];
        m_tex1d_grid_center_lat = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_R32F, GL_STATIC_DRAW));
        m_tex1d_grid_center_lat->setName("center_lat");
        m_tex1d_grid_center_lat->upload(nCenters*sizeof(float), nCenters, varCenterLat->data());
    }
    shader_triangle->SetSamplerUniform("center_lat", &*m_tex1d_grid_center_lat);
    shader_resample->SetSamplerUniform("center_lat", &*m_tex1d_grid_center_lat);
    shader_hexagon->SetSamplerUniform("center_lat", &*m_tex1d_grid_center_lat);
    shader_dvr->SetSamplerUniform("center_lat", &*m_tex1d_grid_center_lat);
    shader_dvr_regular->SetSamplerUniform("center_lat", &*m_tex1d_grid_center_lat);

    if (!m_tex1d_grid_center_lon)
    {
        int nCenters = (int) varCenterLon->m_dimsizes[0];
        m_tex1d_grid_center_lon = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_R32F, GL_STATIC_DRAW));
        m_tex1d_grid_center_lon->setName("center_lon");
        m_tex1d_grid_center_lon->upload(nCenters*sizeof(float), nCenters, varCenterLon->data());
    }
    shader_triangle->SetSamplerUniform("center_lon", &*m_tex1d_grid_center_lon);
    shader_resample->SetSamplerUniform("center_lon", &*m_tex1d_grid_center_lon);
    shader_hexagon->SetSamplerUniform("center_lon", &*m_tex1d_grid_center_lon);
    shader_dvr->SetSamplerUniform("center_lon", &*m_tex1d_grid_center_lon);
    shader_dvr_regular->SetSamplerUniform("center_lon", &*m_tex1d_grid_center_lon);

    if (!m_tex1d_grid_corner_lat)
    {
        int nCorners = (int) varCornerLat->m_dimsizes[0];
        m_tex1d_grid_corner_lat = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_R32F, GL_STATIC_DRAW));
        m_tex1d_grid_corner_lat->setName("corner_lat");
        m_tex1d_grid_corner_lat->upload(nCorners*sizeof(float), nCorners, varCornerLat->data());
        //find min and max
        float *ptr = (float*) varCornerLat->data();
        float minLat = std::numeric_limits<float>::max();
        float maxLat = std::numeric_limits<float>::lowest();
        for (int i = 0; i < nCorners; i++)
        {
            minLat = min(minLat, ptr[i]);
            maxLat = max(maxLat, ptr[i]);
        }
        m_minmax_lat[0] = minLat;
        m_minmax_lat[1] = maxLat;
    }
    cout << "lat range:[" << m_minmax_lat << endl;
    shader_triangle->SetSamplerUniform("corner_lat", &*m_tex1d_grid_corner_lat);
    shader_hexagon->SetSamplerUniform("corner_lat", &*m_tex1d_grid_corner_lat);

    if (!m_tex1d_grid_corner_lon)
    {
        int nCorners = (int) varCornerLon->m_dimsizes[0];
        m_tex1d_grid_corner_lon = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_R32F, GL_STATIC_DRAW));
        m_tex1d_grid_corner_lon->setName("corner_lon");
        m_tex1d_grid_corner_lon->upload(nCorners*sizeof(float), nCorners, varCornerLon->data());
        //find min and max
        float *ptr = (float*) varCornerLon->data();
        float minLon = std::numeric_limits<float>::max();
        float maxLon = std::numeric_limits<float>::lowest();
        for (int i = 0; i < nCorners; i++)
        {
            minLon = min(minLon, ptr[i]);
            maxLon = max(maxLon, ptr[i]);
        }
        m_minmax_lon[0] = minLon;
        m_minmax_lon[1] = maxLon;
    }
    cout << "lon range:[" << m_minmax_lon << endl;

    shader_triangle->SetSamplerUniform("corner_lon", &*m_tex1d_grid_corner_lon);
    shader_hexagon->SetSamplerUniform("corner_lon", &*m_tex1d_grid_corner_lon);

    if (!m_tex1d_grid_center_val)
    {
        int nTimes = (int) varClimate->m_dimsizes[0];
        int nCenters = (int) varClimate->m_dimsizes[1];
        int nLayers = (int) varClimate->m_dimsizes[2];
        shader_triangle->SetIntUniform("nLayers", nLayers);
        shader_hexagon->SetIntUniform("nLayers", nLayers);
        shader_resample->SetIntUniform("nLayers", nLayers);
        shader_dvr->SetIntUniform("nLayers", nLayers);
        shader_dvr_regular->SetIntUniform("nLayers", nLayers);
        assert(nCenters == varCenterLat->m_dimsizes[0]);
        m_tex1d_grid_center_val = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_R32F, GL_STATIC_DRAW));
        m_tex1d_grid_center_val->setName("center_val");
        m_tex1d_grid_center_val->upload(nTimes*nCenters*nLayers*sizeof(float), nTimes*nCenters*nLayers, varClimate->data());
        //dump one layer of scalar value
        /*
        std::vector<float> dump(nCenters, 0.0f);
        const float* src = reinterpret_cast<const float*>(varClimate->data());
        for (int i = 0; i < nCenters; i++)
        {
            dump[i] = src[i*nLayers];
        }
        string ofile = "vorticity.bin";
        std::ofstream ofs(ofile, ios::binary);
        if (!ofs){
            davinci::GLError::ErrorMessage("Cannot open file:" + ofile);
        }
        int nsize = dump.size();
        ofs.write(reinterpret_cast<char*>(&nsize), sizeof(nsize));
        ofs.write(reinterpret_cast<char*>(dump.data()), sizeof(float) * dump.size());
        davinci::GLError::ErrorMessage(string("dump scalar value"));
        ofs.close();
        */
    }
    shader_triangle->SetSamplerUniform("center_val", &*m_tex1d_grid_center_val);
    shader_resample->SetSamplerUniform("center_val", &*m_tex1d_grid_center_val);
    shader_hexagon->SetSamplerUniform("center_val", &*m_tex1d_grid_center_val);
    shader_dvr->SetSamplerUniform("center_val", &*m_tex1d_grid_center_val);

    if (!m_tex1d_grid_cell_corners)
    {
        int nCells = (int) varCellCorners->m_dimsizes[0];
        int nDegrees = (int) varCellCorners->m_dimsizes[1];
        assert(nDegrees == 6);
        m_tex1d_grid_cell_corners = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_R32I, GL_STATIC_DRAW));
        m_tex1d_grid_cell_corners->setName("cell_corners");
        m_tex1d_grid_cell_corners->upload(nCells*nDegrees*sizeof(int), nCells*nDegrees, varCellCorners->data());
    }
    shader_triangle->SetSamplerUniform("cell_corners", &*m_tex1d_grid_cell_corners);
    shader_hexagon->SetSamplerUniform("cell_corners",  &*m_tex1d_grid_cell_corners);
    shader_dvr->SetSamplerUniform("cell_corners",	   &*m_tex1d_grid_cell_corners);

    if (!m_tex1d_grid_corner_cells)
    {
        int nCorners = (int) varCornerCells->m_dimsizes[0];
        int nDegrees = (int) varCornerCells->m_dimsizes[1];
        assert(nDegrees == 3);
        //m_tex1d_grid_corner_cells = GLTexture1DRef(new GLTexture1d(nCorners * nDegrees, GL_RGB32I, GL_RGB_INTEGER, GL_INT, varCornerCells->data()));
        m_tex1d_grid_corner_cells = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_RGB32I, GL_STATIC_DRAW));
        m_tex1d_grid_corner_cells->setName("corner_cells");
        m_tex1d_grid_corner_cells->upload(nCorners*nDegrees*sizeof(int), nCorners*nDegrees, varCornerCells->data());
    }
    shader_triangle->SetSamplerUniform("corner_cells", &*m_tex1d_grid_corner_cells);
    shader_resample->SetSamplerUniform("corner_cells", &*m_tex1d_grid_corner_cells);
    //shader_hexagon->SetSamplerUniform("corner_cells", &*m_tex1d_grid_corner_cells);
    shader_dvr->SetSamplerUniform("corner_cells", &*m_tex1d_grid_corner_cells);
    shader_dvr_regular->SetSamplerUniform("corner_cells", &*m_tex1d_grid_corner_cells);

    if (!m_tex1d_grid_corner_edges)
    {
        int nCorners = (int) varCornerEdges->m_dimsizes[0];
        int nDegrees = (int) varCornerEdges->m_dimsizes[1];
        assert(nDegrees == 3);
        //m_tex1d_grid_corner_edges = GLTexture1DRef(new GLTexture1d(nCorners, GL_RGB32I, GL_RGB_INTEGER, GL_INT,varCornerEdges->data()));
        m_tex1d_grid_corner_edges = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_RGB32I, GL_STATIC_DRAW));
        m_tex1d_grid_corner_edges->setName("corner_edges");
        m_tex1d_grid_corner_edges->upload(nCorners*nDegrees*sizeof(int), nCorners*nDegrees, varCornerEdges->data());
    }
    //shader_triangle->SetSamplerUniform("corner_edges", &*m_tex1d_grid_corner_edges);
    //shader_hexagon->SetSamplerUniform("corner_edges", &*m_tex1d_grid_corner_edges);
    shader_dvr->SetSamplerUniform("corner_edges", &*m_tex1d_grid_corner_edges);

    if (!m_tex1d_grid_edge_cells)
    {
        int nCorners = (int) varEdgeCells->m_dimsizes[0];
        int nDegrees = (int) varEdgeCells->m_dimsizes[1];
        assert(nDegrees == 2);
        m_tex1d_grid_edge_cells = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_RG32I, GL_STATIC_DRAW));
        m_tex1d_grid_edge_cells->setName("edge_cells");
        m_tex1d_grid_edge_cells->upload(nCorners*nDegrees*sizeof(int), nCorners*nDegrees, varEdgeCells->data());
    }
    //shader_triangle->SetSamplerUniform("edge_cells", &*m_tex1d_grid_edge_cells);
    //shader_hexagon->SetSamplerUniform("edge_cells", &*m_tex1d_grid_edge_cells);
    shader_dvr->SetSamplerUniform("edge_cells", &*m_tex1d_grid_edge_cells);
}

void GCRMMesh::CreateVBOTriangleMesh()
{
    if (!m_tex1d_grid_corner_cells)
    {
        CreateTBOConnectivity();
    }

    if (!m_vbo_triangle_center_id)
    {//triangle center id is its dual hexagon cell corner id.
        HostFloatArrayRef	varCornerLat = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lat"];
        m_vbo_triangle_center_id = GLVertexBufferObjectRef(new GLVertexBufferObject());
        int nCorners = (int) varCornerLat->m_dimsizes[0];
        vector<int> cornerIds(nCorners);
        std::iota(cornerIds.begin(), cornerIds.end(), 0);//0,1,2,3,...,nCorners-1
        m_vbo_triangle_center_id->upload(cornerIds.size()*sizeof(int),cornerIds.size(), cornerIds.data());
    }
     
    if (!m_vao_triangle_center_id)
    {
        m_vao_triangle_center_id = GLVertexArrayObjectRef(new GLVertexArrayObject(GL_POINTS));
        GLShaderRef shader = m_shaders["gcrm_triangle_mesh"];
        assert(shader);
        GLint progId = shader->getProgramId();
        m_vao_triangle_center_id->addAttribute(progId,"in_corner_id",GL_INT, 1, sizeof(int),0,false, m_vbo_triangle_center_id);
        m_vao_triangle_center_id->enable();
    }
}

void GCRMMesh::volumeRender()
{
    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);

    if (!m_vao_triangle_center_id)
    {
        CreateVBOTriangleMesh();
    }

    /*
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);

    glClearColor(m_bgColor3f.x(), m_bgColor3f.y(), m_bgColor3f.z(), 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    */

    //ConfigManagerRef config = ConfigManager::getInstance();

    GLShaderRef gridShader = m_shaders["gcrm_dvr"];//direct ray-casting through prism grid using my proposed method.
    /*
    GLShaderRef gridShader = m_shaders["gcrm_dvr_regular"];//ray-casting resampled volume data.
    */
   
    //m_volumeRender->m_worldMapTextures[0]->bind();//necessary??
    //m_volumeRender->m_worldMapTextures[1]->bind();//necessary??
    //if (m_tex3d)
    {
        mat4 inv_mvm = GLContext::g_MVM;
        inv_mvm.inverse();
        gridShader->SetMatrixUniform("modelViewMat", GLContext::g_MVM);
        gridShader->SetMatrixUniform("projMat", GLContext::g_PjM);
        gridShader->SetMatrixUniform("inv_mvm", inv_mvm);
        gridShader->SetBoolUniform("enableLight", true);
        gridShader->SetFloat4Uniform("e_lightPos", GLContext::g_lights[0].m_pos);
        gridShader->UseShaders();//use();

        m_vao_triangle_center_id->draw();

        gridShader->ReleaseShader();
    }

    glPopAttrib();
}

void GCRMMesh::CreateVBOHexagonMesh()
{
    if (!m_tex1d_grid_corner_cells)
    {
        CreateTBOConnectivity();
    }

    if (!m_vbo_hexagon_center_id)
    {//triangle center id is its dual hexagon cell corner id.
        HostFloatArrayRef	varCenterLat = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lat"];
        m_vbo_hexagon_center_id = GLVertexBufferObjectRef(new GLVertexBufferObject());
        int nCenters = (int) varCenterLat->m_dimsizes[0];
        vector<int> center_ids(nCenters);
        std::iota(center_ids.begin(), center_ids.end(), 0);
        m_vbo_hexagon_center_id->upload(center_ids.size()*sizeof(int), center_ids.size(), center_ids.data());
    }

    if (!m_vao_hexagon_center_id)
    {
        m_vao_hexagon_center_id = GLVertexArrayObjectRef(new GLVertexArrayObject(GL_POINTS));
        GLShaderRef shader = m_shaders["gcrm_hexagon_mesh"];
        assert(shader);
        GLint progId = shader->getProgramId();
        m_vao_hexagon_center_id->addAttribute(progId, "in_center_id", GL_INT, 1,
                                sizeof(int), 0, false, m_vbo_hexagon_center_id);
        m_vao_hexagon_center_id->enable();
    }
}

void GCRMMesh::RenderTriangleGridConnc(bool bFillMesh, bool bLighting)
{

    glPushAttrib(GL_ENABLE_BIT);

    if (!m_vao_triangle_center_id)
    {
        CreateVBOTriangleMesh();
    }

    /*
    if (bFillMesh)
    {
        //cout << __func__ << " bFillMesh=" << bFillMesh << endl;
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    */
    /*
    glDisable(GL_CULL_FACE);
    */
    /*
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    */

    NetCDFFileRef climateVariableFile = (*m_netmngr)[m_climateVariableFileName];
    HostFloatArrayRef	varClimate = climateVariableFile->getFloatArray()[m_climateVariableUniqueId];
    int nLayers = (int) varClimate->m_dimsizes[2];
    m_maxIdxLayer = min(m_maxIdxLayer, nLayers-1);
    /**comment for ODS
    glClearColor(m_bgColor3f.x(), m_bgColor3f.y(), m_bgColor3f.z(), 1.0);//
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    */
    GLShaderRef gridShader = m_shaders["gcrm_triangle_mesh"];//m_volumeRender->GetShader("grid_gcrm");
    //m_volumeRender->m_worldMapTextures[0]->bind();//necessary??
    //m_volumeRender->m_worldMapTextures[1]->bind();//necessary??

    gridShader->SetMatrixUniform("modelViewMat", GLContext::g_MVM);
    gridShader->SetMatrixUniform("projMat", GLContext::g_PjM);
    gridShader->SetMatrixUniform("normalMat", GLContext::g_NM);
    //gridShader->SetSamplerUniform("uWorldColorMap",&*(m_volumeRender->m_worldMapTextures[0]) );
    //gridShader->SetSamplerUniform("uWorldAlphaMap",&*(m_volumeRender->m_worldMapTextures[1]) );
    gridShader->SetBoolUniform("enableLight", bLighting);
    gridShader->SetFloat4Uniform("e_lightPos", GLContext::g_lights[0].m_pos);
    gridShader->SetIntUniform("iLayers", m_maxIdxLayer);
    gridShader->UseShaders();//use();

    m_vao_triangle_center_id->draw();

    //m_volumeRender->m_worldMapTextures[0]->unbind();
    //m_volumeRender->m_worldMapTextures[1]->unbind();

    //glDisable(GL_DEPTH_TEST);
    //glDisable(GL_CULL_FACE);

    gridShader->ReleaseShader();

    glPopAttrib();
}
void GCRMMesh::RenderHexagonalGridConnc(bool bFillMesh, bool bLighting)
{
    glPushAttrib(GL_ENABLE_BIT);

    if (!m_vao_hexagon_center_id)
    {
        CreateVBOHexagonMesh();
    }
    //ConfigManagerRef config = ConfigManager::getInstance();
    GLShaderRef gridShader = m_shaders["gcrm_hexagon_mesh"];
    /**Comment for ODS
    if (bFillMesh)
    {
        //cout << __func__ << " bFillMesh=" << bFillMesh << endl;
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    */

    NetCDFFileRef climateVariableFile = (*m_netmngr)[m_climateVariableFileName];
    HostFloatArrayRef	varClimate = climateVariableFile->getFloatArray()[m_climateVariableUniqueId];
    int nLayers = (int) varClimate->m_dimsizes[2];
    m_maxIdxLayer = min(m_maxIdxLayer, nLayers - 1);

    //glClearColor(m_bgColor3f.x(), m_bgColor3f.y(), m_bgColor3f.z(), 1.0);//
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gridShader->SetMatrixUniform("modelViewMat", GLContext::g_MVM);
    gridShader->SetMatrixUniform("projMat",		 GLContext::g_PjM);
    gridShader->SetMatrixUniform("normalMat",	 GLContext::g_NM);
    gridShader->SetBoolUniform("enableLight", bLighting);
    gridShader->SetFloat4Uniform("e_lightPos", GLContext::g_lights[0].m_pos);
    gridShader->SetIntUniform("iLayers", m_maxIdxLayer);
    //gridShader->SetFloatUniform("timing", m_timing);

    gridShader->UseShaders();//use();

    m_vao_hexagon_center_id->draw();

    gridShader->ReleaseShader();

    glPopAttrib();
}

GLVertexBufferObjectRef GCRMMesh::CreateVBOTriangle(){
    //The geometry of the grid is upload only once.
    cout<<"Create VBO for Visible Surface ID\n";
    
    NetCDFFileRef gridFile = (*m_netmngr)[m_gridSimpleFileName];
    NetCDFFileRef climateVariableFile = (*m_netmngr)[m_climateVariableFileName];

    if(!gridFile || !climateVariableFile)
    {
        GLError::ErrorMessage(string(__func__)+m_climateVariableFileName+"is not load!");
    }
    HostIntArrayRef	  varCornerCells = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["corner_cells"];
    HostFloatArrayRef	varCenterLat = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lat"];
    HostFloatArrayRef	varCenterLon = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lon"];
    HostFloatArrayRef	varClimate = climateVariableFile->getFloatArray()[m_climateVariableUniqueId];
    if(!varClimate || ! varCenterLat || !varCenterLon || !varCenterLon //	|| !varInterfaces || !varLayers
        )
    {
        GLError::ErrorMessage(string(__func__)+string(": varCornerCells, varCenterLat, varCenterLon , varNEdgesOnCells or varClimate is NULL!"));
    }

    vec3f v[3];
    int nCorners = (int) (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lat"]->m_dimsizes[0];
    vector<vec3f> verticesAttributes;
    for (int iTriangle = 0 ; iTriangle < nCorners ; ++iTriangle)
    {
        for (int i=0 ; i < 3 ; ++i)
        {//get three centers of the neighborhood hexagon cells.
            //which are the vertex of the triangle we are about to draw later.
            int	  iCell = (*varCornerCells)[iTriangle * 3+i];
            float lat  = (*varCenterLat)[iCell];
            float lon  = (*varCenterLon)[iCell];
            verticesAttributes.push_back(vec3f(lat,lon,float(iTriangle)));
        }
    }//end of for.
    m_vboSurfaceTriangleVtxPosCellId = GLVertexBufferObjectRef(new GLVertexBufferObject());

    m_vboSurfaceTriangleVtxPosCellId->upload(verticesAttributes.size()* sizeof(verticesAttributes[0])
        , verticesAttributes.size(), verticesAttributes.data());

    return m_vboSurfaceTriangleVtxPosCellId;
}

GLIndexBufferObjectRef GCRMMesh::CreateIBOHexagon(){

    cout<<"Create VBO and IBO for hexagonal mesh geometry\n";
    NetCDFFileRef climateVariableFile = (*m_netmngr)[m_climateVariableFileName];
    if(!climateVariableFile)
    {
        GLError::ErrorMessage(string(__func__)+m_climateVariableFileName+"is not load!");
    }
    //m_netmngr["temperature_19010101_000000.nc"]->getFloatArray()["temperature_ifc"];
    HostIntArrayRef   varCellCorners = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["cell_corners"];
    HostFloatArrayRef varCornerLat   = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lat"];
    HostFloatArrayRef varCornerLon   = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lon"];
    if(! varCellCorners || !varCornerLat || !varCornerLon //	|| !varInterfaces || !varLayers
        )
    {
        GLError::ErrorMessage(string(__func__)+string(": varCornerCells, varCenterLat, varCenterLon , varNEdgesOnCells is NULL!"));
    }
    
    int nCell = (int) varCellCorners->m_dimsizes[0];
    //vector<Vec3f_INT> vtxPosCellIdArray;
    vector<vec3f> vtxPosCellIdArray(nCell * 6);
    //feed vertex buffer
    m_iboHexagon = GLIndexBufferObjectRef(new GLIndexBufferObject(GL_POLYGON));
    m_iboHexagon->enableRestart(true);
    m_iboHexagon->setRestartIndex(-1);
    size_t count = 0;
    for(int iCell=0; iCell < nCell ; ++iCell)
    {
        for(int i=0; i < 6 ; ++i)
        {
            int vtxIdx	= (*varCellCorners)[iCell*6+i];
            float lat	= (*varCornerLat)[vtxIdx];
            float lon	= (*varCornerLon)[vtxIdx];
            /*
            float x		= GLOBLE_RADIUS*cos(lat)*sin(lon);
            float y		= GLOBLE_RADIUS*sin(lat);
            float z		= GLOBLE_RADIUS*cos(lat)*cos(lon);

            Vec3f_INT attrib;
            attrib.position = vec3f(x,y,z);
            attrib.cellId = iCell+1;
            vtxPosCellIdArray.push_back(attrib);
            */
            vtxPosCellIdArray[count] = vec3f(lat, lon, float(iCell));
            count++;
            (*m_iboHexagon) << iCell*6+i;
        }
        (*m_iboHexagon) << -1;
    }

    m_vboSurfaceHexagonVtxPosCellId = GLVertexBufferObjectRef(new GLVertexBufferObject());

    m_vboSurfaceHexagonVtxPosCellId->upload(vtxPosCellIdArray.size()*sizeof(vtxPosCellIdArray[0])
                                     ,vtxPosCellIdArray.size(),vtxPosCellIdArray.data());
    m_iboHexagon->enable();

    return m_iboHexagon;
   
}

GLVertexBufferObjectRef GCRMMesh::CreateVBOTrianglePos(){
    //The geometry of the grid is upload only once.
    cout<<"Create VBO for Triangle Surface Pos\n";
    
    NetCDFFileRef gridFile = (*m_netmngr)[m_gridSimpleFileName];
    NetCDFFileRef climateVariableFile = (*m_netmngr)[m_climateVariableFileName];

    if(!gridFile || !climateVariableFile)
    {
        GLError::ErrorMessage(string(__func__)+m_climateVariableFileName+"is not load!");
    }
    HostIntArrayRef	  varCornerCells = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["corner_cells"];
    HostFloatArrayRef	varCenterLat = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lat"];
    HostFloatArrayRef	varCenterLon = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lon"];
    HostFloatArrayRef	varClimate = climateVariableFile->getFloatArray()[m_climateVariableUniqueId];
    if(!varClimate || ! varCenterLat || !varCenterLon || !varCenterLon //	|| !varInterfaces || !varLayers
        )
    {
        GLError::ErrorMessage(string(__func__)+string(": varCornerCells, varCenterLat, varCenterLon , varNEdgesOnCells or varClimate is NULL!"));
    }
    //float x,y,z;
    //vec3f v[3];
    int nCorners = (int) (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lat"]->m_dimsizes[0];
    vector<vec2f> vtxPosArray;

    float minLat = std::numeric_limits<float>::max();
    float minLon = minLat;
    float maxLat = std::numeric_limits<float>::lowest();
    float maxLon = maxLat;
    for (int iTriangle = 0 ; iTriangle < nCorners ; ++iTriangle)
    {
        for (int i=0 ; i < 3 ; ++i)
        {//get three centers of the neighborhood hexagon cells.
            //which are the vertex of the triangle we are about to draw later.
            int	  iCell = (*varCornerCells)[iTriangle * 3+i];
            float lat   = (*varCenterLat)[iCell];
            float lon   = (*varCenterLon)[iCell];
            //iLat[i] = lat;
            //iLon[i] = lon;// < 0 ? lon+PI*2.0f : lon;//avoid wrap-up artifacts on determine the clock-wiseness of triangle.
            minLat = std::min(minLat, lat);
            maxLat = std::max(maxLat, lat);
            minLon = std::min(minLon, lon);
            maxLon = std::max(maxLon, lon);
            vtxPosArray.push_back(vec2f(lon,lat));
        }
    }
    //normalize to [0,1]x[0,1]
    int count = (int) vtxPosArray.size();
    vec2f inv_spanLonLat(1.0f/(maxLon-minLon), 1.0f/(maxLat-minLat));
    vec2f minLonLat(minLon, minLat);
    for (int i = 0 ; i < count ; i++)
    {
        vtxPosArray[i] = (vtxPosArray[i]-minLonLat)*inv_spanLonLat;
    }
    m_vboTrianglePos = GLVertexBufferObjectRef(new GLVertexBufferObject());

    m_vboTrianglePos->upload(vtxPosArray.size()* sizeof(vtxPosArray[0])
                            ,vtxPosArray.size(), vtxPosArray.data());

    return m_vboTrianglePos;
}

string GCRMMesh::getUniqueVarName(const string& varFilePath, const string& varName) const
{
    return Dir::basename(varFilePath) + "_" + varName;
}

//Convert from global time id to file id and local time step id.
//return true if the global id hasn't reached to the end.
/*
bool GCRMMesh::fromGlobalTimeIdToLocalId(int gtimeId, int& iF, int &localTimeId)
{
    ConfigManagerRef config = ConfigManager::getInstance();
    int nFiles = config->m_climateVaribleFilePathNames.size();
    int timeOffset=0; int iFile=0;
    bool bWithinRange= false;

    for (iFile = 0 ; iFile < nFiles ; iFile++)
    {
        QString uniqueVarName = getUniqueVarName(iFile);
        vec2i localTimeRange  = m_dataFileTimeStepCounts[uniqueVarName];
        int nLocalTimeSteps = localTimeRange.y() - localTimeRange.x() + 1;

        if (gtimeId >= timeOffset && gtimeId < timeOffset+nLocalTimeSteps)
        {//Detect which data file array to use.
            bWithinRange = true;
            break;
        }
        timeOffset += nLocalTimeSteps;
    }
    if (iFile >= nFiles)
    {
        bWithinRange = false;
    }
    iF = iFile;
    localTimeId = gtimeId - timeOffset;

    return bWithinRange;
}
*/

//triangle ID is encoded in the alpha components of vertex color in m_vboTriangleColor
void GCRMMesh::RenderRemeshedGridWithLayers(bool bFillMesh, bool bLighting)
{//No index buffer used. Only vertex buffer for [Vx,Vy,Vz,Nx,Ny,Nz,Id0,Id1,Id2]
}

void GCRMMesh::ReadGrid(const string& gridFilePath)//, int timeStart, int timeEnd )
{
    vector<string> gridVarNames({
        "cell_corners",
        "cell_edges",
        "edge_corners",
        "grid_corner_lat",
        "grid_corner_lon",
        "grid_center_lat",
        "grid_center_lon",
    });
    IMesh::LoadGridVar(gridFilePath, gridVarNames);// , 0, 1);
}

void GCRMMesh::LoadClimateData(const string& varFilePath, const string& varName, int timeStart/*=0*/, int timeEnd/*=1*/ )
{
    ifstream varfile(varFilePath, ios::binary);
    if (!varfile)
    {
        cerr << (varFilePath+" not found!\n");
        exit(1);
    }

    m_climateVariableFileName = Dir::basename(varFilePath);
    m_climateVariableName = varName;

    NetCDFFileRef varnetcdf = (*m_netmngr)[m_climateVariableFileName];
    if (!varnetcdf)
    {
        varnetcdf = m_netmngr->addNetCDF(varFilePath);
    }

    NetCDF_var* pVarInfo = varnetcdf->GetVarInfo(varName);
    if (pVarInfo == NULL)
    {
        GLError::ErrorMessage(varName + " not exist in file " + m_climateVariableFileName);
    }
    //m_climateVaribleFileName = simpleFileName;
    m_climateVariableName = varName;
    //int time_out=-1;
    HostIntArrayRef		arrayInt;
    HostFloatArrayRef   arrayFloat;
    HostDoubleArrayRef  arrayDouble;
    int actualTimeMax = pVarInfo->dim[0]->length-1;
    timeStart = min(timeStart, actualTimeMax);
    timeEnd   = min(timeEnd, actualTimeMax);
    printf("***********timeMax=%d,timeStart=%d,timeEnd=%d*************\n", actualTimeMax, timeStart, timeEnd);
    assert(timeStart >= 0);
    assert(timeEnd <= actualTimeMax);
    assert(timeStart <= timeEnd);

    int newTimeSize = timeEnd - timeStart + 1;
    int oldTimeSize = 0;
    string uniqueVarName = m_climateVariableFileName + "_" + varName;
    switch (pVarInfo->rh_type)
    {
        case NC_INT:
            arrayInt = varnetcdf->getIntArray()[uniqueVarName];
            if (NULL == arrayInt){
                arrayInt = varnetcdf->addIntArray(varName, uniqueVarName, newTimeSize);
            }
            oldTimeSize = (int) varnetcdf->getIntArray()[uniqueVarName]->m_dimsizes[0];
            if (oldTimeSize < newTimeSize){
                varnetcdf->removeIntArray(uniqueVarName);
                arrayInt = varnetcdf->addIntArray(varName, uniqueVarName, newTimeSize);
            }
            break;
        case NC_FLOAT:
            arrayFloat = varnetcdf->getFloatArray()[uniqueVarName];
            if (NULL == arrayFloat){
                arrayFloat = varnetcdf->addFloatArray(varName, uniqueVarName, newTimeSize);
            }
            oldTimeSize = (int) varnetcdf->getFloatArray()[uniqueVarName]->m_dimsizes[0];
            if (oldTimeSize < newTimeSize){
                varnetcdf->removeFloatArray(uniqueVarName);
                arrayFloat = varnetcdf->addFloatArray(varName, uniqueVarName, newTimeSize);
            }
            break;
        case NC_DOUBLE:
            arrayDouble = varnetcdf->getDoubleArray()[uniqueVarName];
            if (NULL == arrayDouble){
                arrayDouble = varnetcdf->addDoubleArray(varName, uniqueVarName, newTimeSize);
            }
            oldTimeSize = (int) varnetcdf->getDoubleArray()[uniqueVarName]->m_dimsizes[0];
            if (oldTimeSize < newTimeSize){
                varnetcdf->removeDoubleArray(uniqueVarName);
                arrayDouble = varnetcdf->addDoubleArray(varName, uniqueVarName, newTimeSize);
            }
            break;
    }
    //time_out = max(time_out,1);
    varnetcdf->LoadVarData(varName, uniqueVarName, timeStart, timeEnd-timeStart+1, 0, 1, true);
    m_dataFileTimeStepCounts[uniqueVarName] = davinci::vec2i(timeStart, timeEnd);
    m_totalTimeSteps += (std::abs(timeEnd-timeStart)+1);
    m_dataType = pVarInfo->rh_type;
   
}

void GCRMMesh::normalizeClimateDataArray(const string& varName)
{
    //First pass: find global min and max value.
    double gmin_t = std::numeric_limits<double>::max();
    double gmax_t =-std::numeric_limits<double>::max();

    NetCDFFileRef varnetcdf = (*m_netmngr)[m_climateVariableFileName];
    if (!varnetcdf)
    {
        cerr << (string(__func__) + ": Cannot find data array " + varName + " in the netcdf file!\n");
    }

    string uniqueVarName = m_climateVariableFileName + "_" + varName;

    switch (m_dataType)
    {
        case NC_INT:{
            //varnetcdf->getIntArray()[uniqueVarName]->logNormalize();
            HostIntArrayRef curValArray =
                varnetcdf->getIntArray()[uniqueVarName];
            int localMin, localMax;
            curValArray->getMinMax(localMin, localMax);
            gmin_t = std::min(gmin_t, (double)localMin);
            gmax_t = std::max(gmax_t, (double)localMax);
            break;
        }
        case NC_FLOAT:{
            //varnetcdf->getFloatArray()[uniqueVarName]->logNormalize();
            HostFloatArrayRef curValArray =
                varnetcdf->getFloatArray()[uniqueVarName];
            float localMin, localMax;
            curValArray->getMinMax(localMin, localMax);
            gmin_t = std::min(gmin_t, (double)localMin);
            gmax_t = std::max(gmax_t, (double)localMax);
            break;
        }
        case NC_DOUBLE:{
            //varnetcdf->getFloatArray()[uniqueVarName]->logNormalize();
            HostDoubleArrayRef curValArray =
                varnetcdf->getDoubleArray()[uniqueVarName];
            double localMin, localMax;
            curValArray->getMinMax(localMin, localMax);
            gmin_t = std::min(gmin_t, (double)localMin);
            gmax_t = std::max(gmax_t, (double)localMax);
            break;
        }
    }
    cout << "Global Min=" << gmin_t << ", Max=" << gmax_t << endl;
    //Second pass: normalize the array value using the global min/max from pass one.
    //string uniqueVarName = getUniqueVarName(i);
    //string uniqueVarName = m_climateVariableFileName + "_" + varName;

    //string climateVariableFileName = makeSimpleFileName(config->m_climateVaribleFilePathNames[i]);
    string climateVariableFileName = Dir::basename(m_climateVariableFileName);
    varnetcdf = (*m_netmngr)[climateVariableFileName];
    if (!varnetcdf)
    {
        GLError::ErrorMessage(string(__func__) + ": Cannot find " + climateVariableFileName + " in the netcdf file ");
    }
    switch (m_dataType)
    {
        case NC_INT:{
            //varnetcdf->getIntArray()[uniqueVarName]->logNormalize();
            HostIntArrayRef curValArray =
                varnetcdf->getIntArray()[uniqueVarName];
            curValArray->normalize((int)gmin_t, (int)gmax_t);
            break;
        }
        case NC_FLOAT:{
            //varnetcdf->getFloatArray()[uniqueVarName]->logNormalize();
            HostFloatArrayRef curValArray =
                varnetcdf->getFloatArray()[uniqueVarName];
            curValArray->normalize((float)gmin_t, (float)gmax_t);
            break;
        }
        case NC_DOUBLE:{
            //varnetcdf->getFloatArray()[uniqueVarName]->logNormalize();
            HostDoubleArrayRef curValArray =
                varnetcdf->getDoubleArray()[uniqueVarName];
            curValArray->normalize((double)gmin_t, (double)gmax_t);;
            break;
        }
    }
}

void GCRMMesh::SetMaxIdxLayer( int val )
{
    m_maxIdxLayer = val;
    m_vboHexagonScalar = NULL;
    m_vboTriangleScalar = NULL;
}

void GCRMMesh::SetStepSize(float val)
{
    //ConfigManagerRef config = ConfigManager::getInstance();
    GLShaderRef shader = m_shaders["gcrm_dvr"];
    shader->SetFloatUniform("stepsize", val);
    shader = m_shaders["gcrm_dvr_regular"];
    shader->SetFloatUniform("stepsize", val);
}

void GCRMMesh::SetLightParam(vec4f &lightParam)
{
    GLShaderRef shader = m_shaders["gcrm_dvr"];
    shader->SetFloat4Uniform("lightParam", lightParam);

    shader = m_shaders["gcrm_dvr_regular"];
    shader->SetFloat4Uniform("lightParam", lightParam);
}

bool isCCW(vec3f v0, vec3f v1, vec3f v2){
    vec3f v1_0 = v1 - v0;//vec3f(iLon[1],iLat[1],0)-vec3f(iLon[0],iLat[0],0);
    vec3f v2_0 = v2 - v0;//vec3f(iLon[2],iLat[2],0)-vec3f(iLon[0],iLat[0],0);
    if (abs(v1_0.x())>PI)
    {
        v0[0] = v0[0] < 0 ? v0[0]+PI*2.0f : v0[0];
        v1[0] = v1[0] < 0 ? v1[0]+PI*2.0f : v1[0];
    }
    if (abs(v2_0.x())>PI)
    {
        v2[0] = v2[0] < 0 ? v2[0]+PI*2.0f : v2[0];
    }
    v1_0 = v1 - v0;
    v2_0 = v2 - v0; 
    vec3f v3 = v1_0.cross(v2_0);
    return v3.z() > 0;
}
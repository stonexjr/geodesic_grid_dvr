#include "GCRMMesh.h"
#include "GL/glew.h"
//#include "GL/glut.h"
#include <chrono>
#include <numeric>
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
    m_bDoneResampling = false;
    m_split_range = vec2f(0.0);
}

GCRMMesh::~GCRMMesh(void)
{
} 

GLShaderRef GCRMMesh::getGridShader()
{
    return m_shaders["grid_gcrm"];//m_volumeRender->GetShader("grid_gcrm");
}

void GCRMMesh::InitTextures()
{
    if (!m_tex2d_worldmap)
    {
        m_tex2d_worldmap = GLTexture2DRef(new GLTexture2d(16,16));
        m_tex2d_worldmap->setName("world map texture");
        m_tex2d_worldmap->LoadTexture("./data/world.topo.bathy.200410.3x5400x2700.jpg");
    }
}

void GCRMMesh::InitShaders()
{
    IMesh::InitShaders();
    InitTextures();

    ConfigManagerRef config = ConfigManager::getInstance();
    LIGHTPARAM l = config->m_lightParam;
    vec4f lightParam(l.Kamb, l.Kdif, l.Kspe, l.Kshi);
    cout << "** lightParam=" << lightParam << endl;
    //mesh rendering shader
    GLShaderRef shader = GLShaderRef(new GLShader("grid_gcrm"));
    shader->loadVertexShaderFile(qPrintable(config->m_shaderFiles[0]));
    shader->loadGeomShaderFile(qPrintable(config->m_shaderFiles[1]));
    shader->loadFragShaderFile(qPrintable(config->m_shaderFiles[2]));
    shader->CreateShaders();
    shader->SetIntUniform("enableLight", config->m_bEnableLighting);
    shader->SetIntUniform("drawLine", 0);
    shader->SetIntUniform("drawWorldMap", 0);
    shader->SetIntUniform("max_invisible_cell_id", 0);
    shader->SetFloatUniform("timing", 0);
    shader->SetFloatUniform("ratio", 1.0);
    m_shaders[shader->getShaderName()] = shader;
     
    //shader for rendering mesh directly from connectivity texture
    //dual triangular mesh
    shader = GLShaderRef(new GLShader("gcrm_triangle_mesh", true));
    shader->loadVertexShaderFile(qPrintable(config->m_shaderFiles[3]));
    shader->loadGeomShaderFile(qPrintable(config->m_shaderFiles[4]));
    shader->loadFragShaderFile(qPrintable(config->m_shaderFiles[5]));
    shader->CreateShaders();
    shader->SetIntUniform("enableLight", config->m_bEnableLighting);
    shader->SetIntUniform("drawLine", 0); 
    shader->SetIntUniform("drawWorldMap", 0);
    shader->SetIntUniform("max_invisible_cell_id", 0);
    shader->SetFloatUniform("timing", 0.0);
    shader->SetFloatUniform("ratio", 1.0);
    m_shaders[shader->getShaderName()] = shader; 

    //hexagon mesh 
    shader = GLShaderRef(new GLShader("gcrm_hexagon_mesh", true));
    shader->loadVertexShaderFile(qPrintable(config->m_shaderFiles[6]));
    if (m_bFillMesh)
    {
        shader->loadGeomShaderFile(qPrintable(config->m_shaderFiles[8]));//solid fill
    }else{
        shader->loadGeomShaderFile(qPrintable(config->m_shaderFiles[7]));//frame
    }
    shader->loadFragShaderFile(qPrintable(config->m_shaderFiles[9]));
    shader->CreateShaders();
    shader->SetIntUniform("enableLight", config->m_bEnableLighting);
    shader->SetIntUniform("drawLine", 0);
    shader->SetIntUniform("drawWorldMap", 0);
    shader->SetIntUniform("max_invisible_cell_id", 0);
    shader->SetFloatUniform("timing", 0.0);
    shader->SetFloatUniform("ratio", 1.0);
    m_shaders[shader->getShaderName()] = shader;

    //volume ray casting using my proposed method
    shader = GLShaderRef(new GLShader("gcrm_dvr", true));
    shader->loadVertexShaderFile(qPrintable(config->m_shaderFiles[3]));//TODO
    shader->loadGeomShaderFile(qPrintable(config->m_shaderFiles[10]));
    shader->loadFragShaderFile(qPrintable(config->m_shaderFiles[11]));
    shader->CreateShaders();
    shader->SetIntUniform("enableLight", config->m_bEnableLighting);
    shader->SetFloatUniform("stepsize", 0.01);
    shader->SetFloat4Uniform("lightParam", lightParam);
    //shader->SetIntUniform("drawLine", 0); 
    //shader->SetIntUniform("drawWorldMap", 0);
    //shader->SetIntUniform("max_invisible_cell_id", 0);
    //shader->SetFloatUniform("timing", 0.0);
    //shader->SetFloatUniform("ratio", 1.0);
    m_shaders[shader->getShaderName()] = shader; 

    //resampling shader
    shader = GLShaderRef(new GLShader("resampling", true));
    shader->loadVertexShaderFile(qPrintable(config->m_shaderFiles[13]));
    shader->loadGeomShaderFile(qPrintable(config->m_shaderFiles[14]));
    shader->loadFragShaderFile(qPrintable(config->m_shaderFiles[15]));
    shader->CreateShaders();
    m_shaders[shader->getShaderName()] = shader;

    //dvr regular shader
    shader = GLShaderRef(new GLShader("gcrm_dvr_regular", true));
    shader->loadVertexShaderFile(qPrintable(config->m_shaderFiles[3]));
    shader->loadGeomShaderFile(qPrintable(config->m_shaderFiles[10]));
    shader->loadFragShaderFile(qPrintable(config->m_shaderFiles[12]));
    shader->CreateShaders();
    shader->SetFloatUniform("stepsize", 0.01);
    shader->SetFloat4Uniform("lightParam", lightParam);
    shader->SetFloat2Uniform("split_range", m_split_range);
    shader->SetSamplerUniform("texworldmap", &*m_tex2d_worldmap);
    m_shaders[shader->getShaderName()] = shader;

    //clickable shader is used for drawing cell id 
    //and pixel position and support mouse click query.
    setClickableVtxShader(qPrintable(config->m_shaderFiles[16]));
    setClickableFragShader(qPrintable(config->m_shaderFiles[17]));
    m_clickableShader.setShaderName("GCRM clickable Shader");
    initClickableShaders();
}

void GCRMMesh::refresh()
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
        ConfigManagerRef config = ConfigManager::getInstance();
        GLShaderRef shader = m_shaders["gcrm_hexagon_mesh"];
        if (m_bFillMesh)
        {
            shader->loadGeomShaderFile(qPrintable(config->m_shaderFiles[8]));//solid fill
        }
        else{
            shader->loadGeomShaderFile(qPrintable(config->m_shaderFiles[7]));//frame
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
    IMesh::InitShaders();
    GLShaderRef shader = m_shaders["grid_gcrm"];//m_volumeRender->GetShader("grid_gcrm");
    if (!shader)
    {
        GLError::ErrorMessage(string(__func__)+string("Please initialize gridShader by calling GCRMMesh::InitShaders()!\n"));
    }
    shader->SetSamplerUniform("tf", &*m_tfTex);

    shader = m_shaders["gcrm_triangle_mesh"];
    shader->SetSamplerUniform("tf", &*m_tfTex);

    shader = m_shaders["gcrm_hexagon_mesh"];
    shader->SetSamplerUniform("tf", &*m_tfTex);
    
    shader = m_shaders["gcrm_dvr"];
    shader->SetSamplerUniform("tf", &*m_tfTex);

    shader = m_shaders["resampling"];
    shader->SetSamplerUniform("tf", &*m_tfTex);
    
    shader = m_shaders["gcrm_dvr_regular"];
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
    int nCornerPerEdge = varEdgeCorners->m_dimsizes[1];//
    assert(nCornerPerEdge == 2);
    int nEdgesPerCorner = 3;
    int nEdges = varEdgeCorners->m_dimsizes[0];
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
    int nEdgesPerCell = varCellEdges->m_dimsizes[1];
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

void GCRMMesh::resize( int scrWidth, int scrHeight )
{
    /*
    scrWidth  = 1024;
    scrHeight = 1024;
    */
    //GLClickable::resize(scrWidth, scrHeight);

    //resize the cell id texture and depth texture in GLClickable::resize();
    m_scrWidth = scrWidth;
    m_scrHeight= scrHeight;
    GLClickable::resize(scrWidth, scrHeight);

    if(!m_fboVisSurface) m_fboVisSurface = davinci::GLFrameBufferObjectRef(new davinci::GLFrameBufferObject()); 

    m_texPixelPos = davinci::GLTexture2DRef(new GLTexture2d(scrWidth,scrHeight,
                                            GL_RGBA32F, GL_RGBA, GL_FLOAT));//GL_DOUBLE for MPAS grid.

    m_texPixelPos->setName("m_texPixelPos");
    m_texTriaCellId = davinci::GLTexture2DRef(new GLTexture2d(scrWidth,scrHeight,
                                            GL_R32I, GL_RED_INTEGER, GL_INT));
    m_texTriaCellId->setName("m_texTriaCellId");

    m_fboVisSurface->bind();
    m_fboVisSurface->attachColorBuffer(0, m_texTriaCellId);
    m_fboVisSurface->attachColorBuffer(1, m_texPixelPos);
    m_fboVisSurface->attachDepthBuffer(GLClickable::getDepthTexture());
    m_fboVisSurface->unbind();

    m_fboVisSurface->checkFramebufferStatus();

    m_fragPos.resize(scrWidth*scrHeight*4);
    std::fill(m_fragPos.begin(), m_fragPos.end(), 0.0f);
    //
    m_tex2d = GLTexture2DRef(new GLTexture2d(scrWidth, scrHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT));
    m_tex2d->setName("resampling m_tex2d");
    m_tex2d->setUseFixedPipeline(true);
}

#define VERTATTRIB_T vec3f
//Vec3f_INT
#define GL_FORMAT_T GL_FLOAT
#define REAL_T float
#define REAL3D_T vec3f

void GCRMMesh::drawIDTriangle()
{
    if(!m_vboSurfaceTriangleVtxPosCellId)
    {
        m_vboSurfaceTriangleVtxPosCellId = GCRMMesh::CreateVBOTriangle();
    }

    if (!m_pVAOTrianglePos)
    {
        m_pVAOTrianglePos= GLVertexArrayObjectRef(new GLVertexArrayObject(GL_TRIANGLES));

        GLuint shaderid = m_clickableShader.getProgramId();
        m_pVAOTrianglePos->addAttribute(shaderid,"in_Position", GL_FORMAT_T, 3, 
                                        sizeof(Vec3f_INT),
                                        offsetof(Vec3f_INT, position),
                                        false, m_vboSurfaceTriangleVtxPosCellId);

        m_pVAOTrianglePos->addAttribute(shaderid,"in_CellId", GL_INT, 1, 
                                        sizeof(Vec3f_INT),
                                        offsetof(Vec3f_INT, cellId),
                                        false, m_vboSurfaceTriangleVtxPosCellId);
        /*
        */
    }
    m_pVAOTrianglePos->enable();
    m_pVAOTrianglePos->draw();
}

void GCRMMesh::drawIDHexgon()
{
    if(!m_vboSurfaceHexagonVtxPosCellId || !m_iboHexagon)
    {
        m_iboHexagon = CreateIBOHexagon(); 
    }
    if (!m_pVAOHexagonGridsVtxPos)
    {
        m_pVAOHexagonGridsVtxPos = GLVertexArrayObjectRef(new GLVertexArrayObject(GL_POLYGON));
        GLuint shaderid = m_clickableShader.getProgramId();

        m_pVAOHexagonGridsVtxPos->addAttribute(shaderid,"in_Position", GL_FORMAT_T,3, 
            sizeof(Vec3f_INT), offsetof(Vec3f_INT, position),
            false, m_vboSurfaceHexagonVtxPosCellId);

        m_pVAOHexagonGridsVtxPos->addAttribute(shaderid,"in_CellId", GL_INT,1, 
            sizeof(Vec3f_INT), offsetof(Vec3f_INT, cellId),
            false, m_vboSurfaceHexagonVtxPosCellId);
        /*
        */
    } 

    m_pVAOHexagonGridsVtxPos->enable();
    m_iboHexagon->attachVAO(m_pVAOHexagonGridsVtxPos);
    m_iboHexagon->draw();
}

void GCRMMesh::drawID()
{
    GLenum buffers[] = { 0, 1 };
    m_fboClickable->bind();
    m_fboClickable->enableMultipleRenderTarget(buffers, sizeof(buffers)/sizeof(GLenum));

    m_clickableShader.SetMatrixUniform("modelViewMat", davinci::GLContext::g_MVM);
    m_clickableShader.SetMatrixUniform("projMat",      davinci::GLContext::g_PjM);
    m_clickableShader.UseShaders();

    glPushAttrib(GL_ENABLE_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    /*Disable cull face, since GCRM mesh has CCW and CW triangles alternating.
     *But MPAS mesh should enable cull face since all triangles in MPAS are
     *CCW and we don't want to draw mesh behind continent.
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    */
    glClearColor(0, 0, 0, 0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //ConfigManagerRef config = ConfigManager::getInstance();    
    // m_MeshRef->SetDrawMeshType(config.m_bDrawHexagon ? IMesh::HEXAGON_MESH : IMesh::TRIANGLE_MESH);
    if (m_meshType == "hexagon")
    {
        drawIDHexgon();
    }
    else if(m_meshType == "triangle")
    {
        drawIDTriangle();
    }else{
        GLError::ErrorMessage(string(__func__)+"undefined m_meshType value encountered!\n");
    }

    m_clickableShader.ReleaseShader();
    m_fboClickable->unbind();
    glPopAttrib();
}

int GCRMMesh::mouseClicked( int x, int y )
{
    int iCell = GLClickable::mouseClicked(x, y);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    vec4f position;
    /*
    if (GetGridType()==GCRMMESH || 
        GetGridType()==MPASMESH)
    {
    */
        m_fboVisSurface->bind();
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glReadPixels(x, viewport[3] - y, 1, 1, GL_RGBA, GL_FLOAT, &position[0]);
        m_fboVisSurface->unbind();
        glReadBuffer(GL_BACK);

    /*
    }if (GetGridType() == cuGCRMMESH)
    { }
    */

    GLError::glCheckError(__func__"end of readbuffer.");

    if (iCell>=0)
    {
        if(m_meshType == "hexagon")
        {
            ConfigManagerRef config = ConfigManager::getInstance();
            cout << "cell id="<<iCell<<", pixel o_position="<<position;
            //show only hexagon attribute.
            NetCDFFileRef climateVariableFile = (*m_netmngr)[m_climateVariableFileName];
            if(!climateVariableFile) GLError::ErrorMessage(QString(__func__)+m_climateVariableFileName+QString("is not loaded!"));

            QString uniqueVarName = getUniqueVarName(m_curFileStep);
            HostFloatArrayRef varClimate = climateVariableFile->getFloatArray()[uniqueVarName];
            if(!varClimate) GLError::ErrorMessage(QString(__func__)+m_climateVariableUniqueId+QString("is not found!"));

            float val=0.0f;
            int ndim      = varClimate->m_ndim;
            int dim1_size = varClimate->m_dimsizes[0];
            int dim2_size = varClimate->m_dimsizes[1];
            int dim3_size = ndim > 2 ? varClimate->m_dimsizes[2] : 1;
            int time	  = min(m_curLocalTimeStep, dim1_size-1);
            int intface   = min(m_maxIdxLayer, dim3_size-1);//m_maxIdxLayer;
            //cout<<"Current Interface Id="<<intface<<endl;
            if (ndim==3)
            {
                val = (*varClimate)(HostFloatArrayType::DIM3,time,iCell,intface);
            }else if (ndim==2)
            {
                val = (*varClimate)(HostFloatArrayType::DIM2,time,iCell);
            }
            cout << "cell["<<iCell<<"].val="<<val<<endl;
        }else if (m_meshType == "triangle")
        {
            cout << "triangle id="<<iCell<<", pixel o_position="<<position;
            NetCDFFileRef climateVariableFile = (*m_netmngr)[m_climateVariableFileName];
            if(!climateVariableFile) GLError::ErrorMessage(QString(__func__)+m_climateVariableFileName+QString("is not loaded!"));

            NetCDFFileRef gridFile = (*m_netmngr)[m_gridSimpleFileName];
            HostIntArrayRef	  varCornerCells = gridFile->getIntArray()["corner_cells"];
            HostFloatArrayRef	varCenterLat = gridFile->getFloatArray()["grid_center_lat"];
            HostFloatArrayRef	varCenterLon = gridFile->getFloatArray()["grid_center_lon"];
            HostFloatArrayRef	varClimate   = climateVariableFile->getFloatArray()[m_climateVariableUniqueId];
            if (!varCornerCells || !varCenterLat || !varCenterLon || !varClimate)
            {
                GLError::ErrorMessage(string(__func__)+": varCornerCells, varCenterLat,varCenterLon, varClimate are NULL!");
            }
            int ndim      = varClimate->m_ndim;
            int dim1_size = varClimate->m_dimsizes[0];
            int dim2_size = varClimate->m_dimsizes[1];
            int dim3_size = ndim > 2 ? varClimate->m_dimsizes[2] : 1;
            int time	  = min(m_curLocalTimeStep, dim1_size-1);
            int intface   = min(m_maxIdxLayer, dim3_size-1);//m_maxIdxLayer;
            //List the indices of the triangle vertices.
            int iTriangle = iCell;
            for (int i=0 ; i < 3 ; ++i)
            {//get three centers of the neighborhood hexagon cells.
                //which are the vertex of the triangle we are about to draw later.
                int iCell = (*varCornerCells)[iTriangle * 3+i];
                float val = (*varClimate)(HostFloatArrayType::DIM3,time,iCell,intface);
                float lat  = (*varCenterLat)[iCell];
                float lon  = (*varCenterLon)[iCell];
                cout << "--v"<<i<<"(lon,lat).val=("<<lon<<","<<lat<<").val="<<val<<"\n";
            }
        }

    }
    GLError::glCheckError(__func__"end of.");
    return iCell;
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
        GLError::ErrorMessage(QString(__func__) + m_climateVariableFileName + "is not load!");
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
        int nEdges   = varEdgeCorners->m_dimsizes[0];
        int nDegrees = varEdgeCorners->m_dimsizes[1];
        assert(nDegrees == 2);
        m_tex1d_grid_edge_corners = GLTextureBufferObjectRef(new GLTextureBufferObject(GL_RG32I, GL_STATIC_DRAW));
        m_tex1d_grid_edge_corners->setName("edge_corners");
        m_tex1d_grid_edge_corners->upload(nEdges*nDegrees*sizeof(int), nEdges, varEdgeCorners->data());
    }
    shader_dvr->SetSamplerUniform("edge_corners", &*m_tex1d_grid_edge_corners);

    if (!m_tex1d_grid_center_lat)
    {
        int nCenters = varCenterLat->m_dimsizes[0];
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
        int nCenters = varCenterLon->m_dimsizes[0];
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
        int nCorners = varCornerLat->m_dimsizes[0];
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
        int nCorners = varCornerLon->m_dimsizes[0];
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
        int nTimes = varClimate->m_dimsizes[0];
        int nCenters = varClimate->m_dimsizes[1];
        int nLayers = varClimate->m_dimsizes[2];
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
        int nCells = varCellCorners->m_dimsizes[0];
        int nDegrees = varCellCorners->m_dimsizes[1];
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
        int nCorners = varCornerCells->m_dimsizes[0];
        int nDegrees = varCornerCells->m_dimsizes[1];
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
        int nCorners = varCornerEdges->m_dimsizes[0];
        int nDegrees = varCornerEdges->m_dimsizes[1];
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
        int nCorners = varEdgeCells->m_dimsizes[0];
        int nDegrees = varEdgeCells->m_dimsizes[1];
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
        int nCorners = varCornerLat->m_dimsizes[0];
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

    ConfigManagerRef config = ConfigManager::getInstance();

    /*
    GLShaderRef gridShader = m_shaders["gcrm_dvr"];//direct ray-casting through prism grid using my proposed method.
    */
    GLShaderRef gridShader = m_shaders["gcrm_dvr_regular"];//ray-casting resampled volume data.
    //Resampling(1600, 800);
    //Resampling(2048, 2048);
    //Resampling(1024,1024);
    if (  !m_tex3d 
        || m_tex3d->getWidth()!= m_scrWidth
        || m_tex3d->getHeight()!= m_scrHeight)
    {
        Resampling(256,256);
    }
    //m_volumeRender->m_worldMapTextures[0]->bind();//necessary??
    //m_volumeRender->m_worldMapTextures[1]->bind();//necessary??
    if (m_tex3d)
    {
        mat4 inv_mvm = GLContext::g_MVM;
        inv_mvm.inverse();
        gridShader->SetMatrixUniform("modelViewMat", GLContext::g_MVM);
        gridShader->SetMatrixUniform("projMat", GLContext::g_PjM);
        gridShader->SetMatrixUniform("inv_mvm", inv_mvm);
        //gridShader->SetSamplerUniform("uWorldColorMap",&*(m_volumeRender->m_worldMapTextures[0]) );
        //gridShader->SetSamplerUniform("uWorldAlphaMap",&*(m_volumeRender->m_worldMapTextures[1]) );
        gridShader->SetBoolUniform("enableLight", config->m_bEnableLighting);
        gridShader->SetFloat4Uniform("e_lightPos", GLContext::g_lights[0].m_pos);
        gridShader->UseShaders();//use();

        m_vao_triangle_center_id->draw();

        //m_volumeRender->m_worldMapTextures[0]->unbind();
        //m_volumeRender->m_worldMapTextures[1]->unbind();

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
        int nCenters = varCenterLat->m_dimsizes[0];
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
    int nLayers = varClimate->m_dimsizes[2];
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
    ConfigManagerRef config = ConfigManager::getInstance();
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
    int nLayers = varClimate->m_dimsizes[2];
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
        GLError::ErrorMessage(QString(__func__)+m_climateVariableFileName+"is not load!");
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

    float x[3], y[3], z[3];
    //float iLat[3], iLon[3];
    vec3f v[3];
    int nCorners = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lat"]->m_dimsizes[0];
    //vector<Vec3f_INT> vtxPosArray;//(nCorners*3);
    vector<vec3f> verticesAttributes;//(lat,lon,triangleId)
    for (int iTriangle = 0 ; iTriangle < nCorners ; ++iTriangle)
    {
        for (int i=0 ; i < 3 ; ++i)
        {//get three centers of the neighborhood hexagon cells.
            //which are the vertex of the triangle we are about to draw later.
            int	  iCell = (*varCornerCells)[iTriangle * 3+i];
            float lat  = (*varCenterLat)[iCell];
            float lon  = (*varCenterLon)[iCell];
            verticesAttributes.push_back(vec3f(lat,lon,float(iTriangle)));
            /*
            x[i] = GLOBLE_RADIUS*cos(lat)*sin(lon);
            y[i] = GLOBLE_RADIUS*sin(lat);
            z[i] = GLOBLE_RADIUS*cos(lat)*cos(lon);
            v[i] = vec3f(lon, lat, 0.0f);
            */
            //iLat[i] = lat;
            //iLon[i] = lon;// < 0 ? lon+PI*2.0f : lon;//avoid wrap-up artifacts on determine the clock-wiseness of triangle.
            /*
            Vec3f_INT attrib;
            attrib.position = vec3f(x[i],y[i],z[i]);
            attrib.cellId = iTriangle+1;
            */
            /*
            vtxPosArray.push_back(vec3f(x[i],y[i],z[i]));
            */
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
        GLError::ErrorMessage(QString(__func__)+m_climateVariableFileName+"is not load!");
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
    
    int nCell = varCellCorners->m_dimsizes[0];
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
        GLError::ErrorMessage(QString(__func__)+m_climateVariableFileName+"is not load!");
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
    int nCorners = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lat"]->m_dimsizes[0];
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
    int count = vtxPosArray.size();
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

davinci::GLTextureAbstract* GCRMMesh::Resampling(int width, int height)
{
    if (m_bDoneResampling) return NULL;

    if (!m_vao_triangle_center_id)
    {
        CreateVBOTriangleMesh();
    }
    
    if (m_bFillMesh)
    {
        cout << __func__ << " bFillMesh=" << m_bFillMesh << endl;
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    NetCDFFileRef climateVariableFile = (*m_netmngr)[m_climateVariableFileName];
    GLShaderRef   resampling_shader = m_shaders["resampling"];
    ConfigManagerRef config = ConfigManager::getInstance();
    HostFloatArrayRef varClimate = climateVariableFile->getFloatArray()[m_climateVariableUniqueId];

    int nLayers = varClimate->m_dimsizes[2];

    vector<float> volume(width*height*nLayers, -1.0f);
    if (  !m_tex3d 
        || m_tex3d->getWidth()!=width 
        || m_tex3d->getHeight()!=height 
        || m_tex3d->getDepth()!=nLayers)
    {
        m_tex3d = GLTexture3DRef(
            new GLTexture3d(width, height, nLayers, GL_R32F, 
                            GL_RED, GL_FLOAT, volume.data())) ;
        //m_tex3d->setWrapMode(GL_REPEAT);
        m_tex3d->setName("resample volume tex3d");
    }

#ifndef DEBUG_RESAMPLING

    if (!m_fboResampling)
        m_fboResampling = davinci::GLFrameBufferObjectRef(new davinci::GLFrameBufferObject());

#else
    nLayers = 1;//just resample one layer.
#endif

    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);

    glClearColor(m_bgColor3f.x(), m_bgColor3f.y(), m_bgColor3f.z(), 1.0);//
    GLContext::glMatrixMode(GLContext::GL_ModelViewMatrix);
    GLContext::glPushMatrix();
    GLContext::g_MVM.identity();
    GLContext::glMatrixMode(GLContext::GL_ProjectionMatrix);
    GLContext::glPushMatrix();
    float ratio = 0.00f;
    float ghost_witdh = (m_minmax_lon[1] - m_minmax_lon[0])*ratio;
    float ghost_height= (m_minmax_lat[1] - m_minmax_lat[0])*ratio;
    GLContext::g_PjM = davinci::mat4::createOrthoProjMatrix(
                            m_minmax_lon[0] - ghost_witdh,
                            m_minmax_lon[1] + ghost_witdh, 
                            m_minmax_lat[0] - ghost_height,
                            m_minmax_lat[1] + ghost_height,
                            -1, 1);
    GLint old_viewport[4];
    glGetIntegerv(GL_VIEWPORT, old_viewport);

    glViewport(0, 0, width, height);
    //glViewport(0, 0, m_scrWidth, m_scrHeight);
#ifndef DEBUG_RESAMPLING
    m_bFillMesh = true;
#endif

    if (m_bFillMesh)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        cout << " fill polygon.\n";
    }else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        cout << " wireframe.\n";
    }

    glClearColor(0, 0, 0, 1);

    resampling_shader->SetMatrixUniform("modelViewMat",  GLContext::g_MVM);
    resampling_shader->SetMatrixUniform("normalMat",	 GLContext::g_NM);
    resampling_shader->SetMatrixUniform("projMat",		 GLContext::g_PjM);

    resampling_shader->SetBoolUniform("remove_wrapover", config->m_remove_wrapover);
    mat4 mvm;
    float worldmap_width = (m_minmax_lon[1] - m_minmax_lon[0]);
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int ilayer = 0; ilayer < nLayers; ilayer++)
    {
        mvm = GLContext::g_MVM;
        resampling_shader->SetIntUniform("iLayers", ilayer);
        resampling_shader->SetMatrixUniform("modelViewMat", mvm);
        resampling_shader->UseShaders();
#ifndef DEBUG_RESAMPLING
        m_fboResampling->bind();
        //Render to texture
        m_fboResampling->attachTextureND(0, *m_tex3d, nLayers-1-ilayer);
        m_fboResampling->checkFramebufferStatus();
#endif

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cout << ilayer << endl;
        m_vao_triangle_center_id->draw();
        //right ghost region
        mvm.translate(worldmap_width,0.0f,0.0f);
        resampling_shader->SetMatrixUniform("modelViewMat", mvm);
        resampling_shader->UseShaders();
        m_vao_triangle_center_id->draw();

        //left ghost region
        mvm = GLContext::g_MVM;
        mvm.translate(-worldmap_width,0.0f,0.0f);
        resampling_shader->SetMatrixUniform("modelViewMat", mvm);
        resampling_shader->UseShaders();
        m_vao_triangle_center_id->draw();
        /*
        */
#ifndef DEBUG_RESAMPLING
        m_fboResampling->unbind();
#endif
        resampling_shader->ReleaseShader();
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = end_time - start_time;
    std::cout << "\n\nUsed " <<
        std::chrono::duration_cast<std::chrono::milliseconds>(time).count()
        << "ms to generate resampled volume["
        <<m_tex3d->getWidth()
        <<"x"<<m_tex3d->getHeight()<<"x"<<m_tex3d->getDepth()<<"] on GPU.\n\n";
#ifndef DEBUG_RESAMPLING
    /*
    m_tex3d->bindTexture();
    glGetTexImage(m_tex3d->getTarget(), 0, m_tex3d->getFormat(),
                  m_tex3d->getType(), volume.data());
    GLError::glCheckError("***");
    m_tex3d->unbindTexture();
    size_t count = 0;
    for (float e: volume)
    {
        if (e>=0.0)
        {
            count++;
        }
    }
    cout << "Total count=" << count << endl;
    string path = "C:/Users/Administrator/Documents/Visual Studio 2010/Projects/gitlibs/mprg/data";
    stringstream ss;
    ss <<path << "/resampled_volume_" << width << "x" << height << "x" << nLayers << ".bin";

    string filename = ss.str();
    ofstream ofs(filename, ios::binary);
    if (!ofs)
    {
        cerr << "Failed to open file: " << filename << endl;
        exit(1);
    }
    ofs.write(reinterpret_cast<char*>(volume.data()), volume.size()*sizeof(float));
    ofs.close();
    cout << "write resampled volume to " << filename << endl;
    */
#endif

    GLContext::glMatrixMode(GLContext::GL_ProjectionMatrix);
    GLContext::glPopMatrix();
    GLContext::glMatrixMode(GLContext::GL_ModelViewMatrix);
    GLContext::glPopMatrix();

    glViewport(old_viewport[0], old_viewport[1], old_viewport[2], old_viewport[3]);
    glPopAttrib();

#ifndef DEBUG_RESAMPLING
    m_bDoneResampling = true;
#endif

    GLShaderRef shader = m_shaders["gcrm_dvr_regular"];
    shader->SetSamplerUniform("tex3dScalar", &*m_tex3d);
    return NULL;// &*m_tex3d;
}

davinci::GLTextureAbstract* GCRMMesh::RenderMeshTo3DTexture2( int width, int height)
{
    NetCDFFileRef gridFile = (*m_netmngr)[m_gridSimpleFileName];
    NetCDFFileRef tempFile = (*m_netmngr)[m_climateVariableFileName];
    //(*m_netmngr)["temperature_19010101_000000.nc"];
    if(!gridFile || !tempFile)
        return NULL;
    int nCorners = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_corner_lat"]->m_dimsizes[0];
    HostIntArrayRef	  varCornerCells = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["corner_cells"];
    HostFloatArrayRef	varCenterLat = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lat"];
    HostFloatArrayRef	varCenterLon = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lon"];
    HostFloatArrayRef	varClimate   = tempFile->getFloatArray()[m_climateVariableUniqueId];
    //HostFloatArrayPtr varInterfaces = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["interfaces"];
    //HostFloatArrayPtr varLayers = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["layers"];
    if(!varCornerCells ||  !varClimate) return NULL;

    //float R = 1.0f;
    int dim1_size=varClimate->m_dimsizes[0];
    int dim2_size=varClimate->m_dimsizes[1];
    int dim3_size=varClimate->m_dimsizes[2];
    int time    = min(m_curLocalTimeStep, dim1_size-1);
    float R = GLOBLE_RADIUS;
    if (!m_vboTrianglePos)
    {
        CreateVBOTrianglePos();
    }
    GLError::glCheckError(string(__func__)+"----");
    if(!m_vboTriangleScalar)
        m_vboTriangleScalar =  GLVertexBufferObjectRef(new GLVertexBufferObject());

    //Create VAO if not created before
    GLShaderRef shader = m_shaders["resampling"];//m_volumeRender->GetShader("resampling");
    if (!shader)
    {
        GLError::ErrorMessage(string(__func__)+"Cannot find shader renderTO3dTexture.");
    }
    if (!m_pVAOTrianglePosScalar)
    {
        m_pVAOTrianglePosScalar = GLVertexArrayObjectRef(new GLVertexArrayObject(GL_TRIANGLES));

        GLuint shaderid = shader->getProgramId();
        m_pVAOTrianglePosScalar->addAttribute(shaderid, "in_Position2D", GL_FLOAT,2, sizeof(vec2f),0,false,m_vboTrianglePos);
        m_pVAOTrianglePosScalar->addAttribute(shaderid, "in_Scalar",   GL_FLOAT,1, sizeof(float),0,false,m_vboTriangleScalar);
    }
    //float x, y, z, 
    m_tex3d = GLTexture3DRef(new GLTexture3d(width, height, dim3_size,GL_R32F,GL_RED, GL_FLOAT,NULL));
    /*
    if(!m_tex2d)
    {
        GLError::ErrorMessage(string(__func__)+"m_tex2d is NULL");
    }
    */

    if(!m_fboResampling)
        m_fboResampling = davinci::GLFrameBufferObjectRef(new davinci::GLFrameBufferObject());

    m_fboResampling->bind();
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    /*
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    */
    GLContext::glMatrixMode(GLContext::GL_ModelViewMatrix);
    GLContext::glPushMatrix();
    GLContext::g_MVM.identity();
    /*
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0,1,0,1,-1,1);
    */
    GLContext::glMatrixMode(GLContext::GL_ProjectionMatrix);
    GLContext::glPushMatrix();
    GLContext::g_PjM = davinci::mat4::createOrthoProjMatrix(0,1,0,1,-1,1);
    glViewport(0,0, m_scrWidth, m_scrHeight);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    shader->SetMatrixUniform("modelViewMat",GLContext::g_MVM);
    shader->SetMatrixUniform("projMat",GLContext::g_PjM);

    shader->UseShaders();
    //dim3_size = 1;
    glClearColor(0,0,0,1);

    vec3f v[3];
    float val[3];//, iLon[3], iLat[3];
    vector<float> triScalarArray;
    for (int iLayer = 0 ; iLayer < dim3_size ; iLayer++)
    {//render each slice of data to a layer of 2D texture in tex3d
        //update VBO using current data slice
        //cerr<<"Update triangular mesh scalar values at layer "<<iLayer<<".\n";
        for (int iTriangle = 0 ; iTriangle < nCorners ; ++iTriangle)
        {
            for (int i=0 ; i < 3 ; ++i)
            {//get three centers of the neighborhood hexagon cells.
                //which are the vertex of the triangle we are about to draw later.
                int iCell = (*varCornerCells)[iTriangle * 3+i];
                val[i]    = (*varClimate)(HostFloatArrayType::DIM3,time,iCell,iLayer);
            }
            triScalarArray.push_back(val[0]);
            triScalarArray.push_back(val[1]);
            triScalarArray.push_back(val[2]);
        }

        m_vboTriangleScalar->upload(triScalarArray.size()*sizeof(triScalarArray[0])
                                   ,triScalarArray.size(), triScalarArray.data());
        //Render to texture
        m_fboResampling->attachTextureND(0, *m_tex3d, iLayer);
        //m_fboResampling->attachTextureND(0, *m_tex2d);
        m_fboResampling->checkFramebufferStatus();
        m_pVAOTrianglePosScalar->enable();
        /*
        if (iLayer==0)
        {
            m_fboResampling->printFramebufferInfo();
        }
        */
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_pVAOTrianglePosScalar->draw();

    }
    shader->ReleaseShader();
    m_pVAOTrianglePosScalar->disable();

    GLContext::glMatrixMode(GLContext::GL_ProjectionMatrix);
    GLContext::glPopMatrix();
    GLContext::glMatrixMode(GLContext::GL_ModelViewMatrix);
    GLContext::glPopMatrix();

    glPopAttrib();
    m_fboResampling->unbind();

    return &*m_tex3d;//m_fboResampling->getAttachedTextureND(0);
}

string GCRMMesh::getUniqueVarName(const string& varFilePath, const string& varName) const
{
    return Dir::basename(varFilePath) + "_" + varName;
}

//Convert from global time id to file id and local time step id.
//return true if the global id hasn't reached to the end.
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

bool GCRMMesh::SetCurrentGlobalTimeSliceId( int timeId )
{
    IMesh::SetCurrentGlobalTimeSliceId(timeId);
    bool bWithinRange = fromGlobalTimeIdToLocalId(m_timeGlobal, m_curFileStep, m_curLocalTimeStep);
    if (!bWithinRange)
    {//if out of range, then simply wrap around to the starting position 
     //to prevent index out of bound problem
        m_timeGlobal = m_timeGlobal % IMesh::GetTotalTimeSteps();
        bWithinRange = fromGlobalTimeIdToLocalId(m_timeGlobal, m_curFileStep, m_curLocalTimeStep);
        assert(bWithinRange);
    }
    m_climateVariableUniqueId = getUniqueVarName(m_curFileStep);
    ConfigManagerRef config = ConfigManager::getInstance();
    m_climateVariableFileName = makeSimpleFileName(config->m_climateVaribleFilePathNames[m_curFileStep]);
    cout << "file step="<<m_curFileStep<<", local time step="<<m_curLocalTimeStep<<endl;
    return bWithinRange;
}

QString GCRMMesh::GetClimateVariableName() const
{
    return getUniqueVarName(m_curFileStep);
}

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

/*
int GCRMMesh::GetVariableTimeSliceCount(const QString& varFileName, const QString& varName)
{
    QString simpleFileName = makeSimpleFileName(varFileName);
    NetCDFFileRef varnetcdf = (*m_netmngr)[simpleFileName];
    NetCDF_var* pVarInfo = varnetcdf->GetVarInfo(varName);
    if (pVarInfo == NULL)
    {
        GLError::ErrorMessage(varName+" not exist in file "+simpleFileName);
    }
    return pVarInfo->dim[0]->length;
}
*/

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
            oldTimeSize = varnetcdf->getIntArray()[uniqueVarName]->m_dimsizes[0];
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
            oldTimeSize = varnetcdf->getFloatArray()[uniqueVarName]->m_dimsizes[0];
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
            oldTimeSize = varnetcdf->getDoubleArray()[uniqueVarName]->m_dimsizes[0];
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

    NetCDFFileRef varnetcdf = (*m_netmngr)[varName];
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
    string uniqueVarName = m_climateVariableFileName + "_" + varName;

    //string climateVariableFileName = makeSimpleFileName(config->m_climateVaribleFilePathNames[i]);
    string climateVariableFileName = Dir::basename(m_climateVariableFileName)
        NetCDFFileRef varnetcdf = (*m_netmngr)[climateVariableFileName];
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

void GCRMMesh::setInVisibleCellId(int maxNcells)
{
    ConfigManagerRef config = ConfigManager::getInstance();
    GLShaderRef shader = m_shaders["gcrm_triangle_mesh"];
    shader->SetIntUniform("max_invisible_cell_id", maxNcells);
    shader = m_shaders["gcrm_hexagon_mesh"];
    shader->SetIntUniform("max_invisible_cell_id", maxNcells);
}

void GCRMMesh::SetTiming(float timing)
{
    m_timing = timing;
    ConfigManagerRef config = ConfigManager::getInstance();
    GLShaderRef shader = m_shaders["gcrm_triangle_mesh"];
    shader->SetFloatUniform("timing", timing);
    shader = m_shaders["gcrm_hexagon_mesh"];
    shader->SetFloatUniform("timing", timing);
}

void GCRMMesh::SetStepSize(float val)
{
    ConfigManagerRef config = ConfigManager::getInstance();
    GLShaderRef shader = m_shaders["gcrm_dvr"];
    shader->SetFloatUniform("stepsize", val);
    shader = m_shaders["gcrm_dvr_regular"];
    shader->SetFloatUniform("stepsize", val);
}

void GCRMMesh::SetLightParam(vec4f &lightParam)
{
    ConfigManagerRef config = ConfigManager::getInstance();
    GLShaderRef shader = m_shaders["gcrm_dvr"];
    shader->SetFloat4Uniform("lightParam", lightParam);

    shader = m_shaders["gcrm_dvr_regular"];
    shader->SetFloat4Uniform("lightParam", lightParam);
}

void GCRMMesh::SetSplitRange(davinci::vec2f range)
{
    cout << "split range=" << range;
    m_split_range = range;
    ConfigManagerRef config = ConfigManager::getInstance();
    GLShaderRef shader = m_shaders["gcrm_dvr_regular"];
    shader->SetFloat2Uniform("split_range", range);
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


/*

float GCRMMesh::GetValue(float3& pos, const GCRMPrism& prism)
{
    float3 baryCenter = prism.vtxCoordTop[0] + prism.vtxCoordTop[1] + prism.vtxCoordTop[2];
    baryCenter /= 3.0f;
    float dist2NorthPole = length(baryCenter - make_float3(0.0f,1.0f,0.0f)) ;
    float dist2SouthPole = length(baryCenter - make_float3(0.0f,-1.0f,0.0f));
    float val = min(dist2NorthPole, dist2SouthPole);
    val /= (1.414f*GLOBLE_RADIUS);//normalize to [0,1]
    return clamp(val, 0.0f, 1.0f);
}
float GCRMMesh::GetValueTest(float3& pos)
{
    float3 volCenter = make_float3(0,0,0);
    return  length(pos - volCenter);
}
float3 GCRMMesh::GetNormal(float3& pos)
{
    return normalize(pos);
}
////////////////For GPU Acceleration//////////////////////////////////////////////////////////
void GCRMMesh::RenderVolumeGPU( uint* d_output, uint* d_input, const float3* d_pixelPosition,
                                 int m_scrWidth, int m_scrHeight, cudaStream_t stream )
{
    GPUVolumeRender* pGPUVolRender = dynamic_cast<GPUVolumeRender*>(&*m_volumeRender);
    if (pGPUVolRender)
    {
        int deviceID=-1; checkCudaErrors(cudaGetDevice(&deviceID));
        if (!pGPUVolRender->UseMultipleGPU()){
            deviceID=0;
            printf("GCRMMesh::RenderVolumeGPU( ); use single GPU, changed deviceID to %d\n",deviceID);
        }
        gcrm_renderkernel(pGPUVolRender->GetGridSize(), pGPUVolRender->GetBlockSize(),
            d_output, d_input, d_pixelPosition, m_scrWidth, m_scrHeight,deviceID,m_interpType, stream);
    }
}

void GCRMMesh::RenderVolumeCPU( uint* h_output, uint* h_input, int imageW, int imageH ){
    RenderPrismVolumeCPU(h_output, h_input, imageW, imageH);
    //RenderTetraVolumeCPU(h_output, h_input, imageW, imageH);
}

float2 GCRMMesh::sphericalMapping( const float3 &position )
{
    float r = length(position);
    float theta = acos(position.y / r);
    float fi = atan2(position.x, position.z)+PI;
    return make_float2(fi * inv2Pi, theta * invPi);
}

void GCRMMesh::bindVisibleSurfaceTexture(){
    m_texTriaCellId->bind();
}
void GCRMMesh::unbindVisibleSurfaceTexture(){
    m_texTriaCellId->unbind();
}
void GCRMMesh::bindVisSurfacePixPositionFBO()
{
    m_fboVisSurface->bind();
}

void GCRMMesh::unbindVisSurfacePixPositionFBO()
{
    m_fboVisSurface->unbind();
}

GLuint GCRMMesh::GetCellIdTexId()
{
    return m_texTriaCellId->getTextureId();
}

GLuint GCRMMesh::GetPixelPositionTexId()
{
    return m_texPixelPos->getTextureId();
}

void GCRMMesh::cuSetClipeRect( const RectInt2D& m_clipeRect )
{
    gcrm_cuSetClipeRect(m_clipeRect);
}

void GCRMMesh::updateHistogram(QTFEditor* tfEditor )
{
    HostFloatArrayRef climateArray = GetFloatArray(GetClimateVariableName());
    int dimSize1 = climateArray->m_dimsizes[0];
    int dimSize2 = climateArray->m_dimsizes[1];
    int dimSize3 = climateArray->m_dimsizes[2];
    size_t dataSize = 1*dimSize2*dimSize3;
    // 		for (int idim = 0; idim < climateArray->m_ndim; idim++){
    // 			dataSize *= climateArray->m_dimsizes[idim];
    // 		}
    tfEditor->getHistogram()->clear();
    int startOffset = GetCurrentGlobalTimeSliceId()*dimSize2*dimSize3;
    for (size_t i = 0 ; i < dataSize ; ++i)
    {
        //m_tfEditor->incrementHistogram((*climateArray)[i]);
        tfEditor->incrementHistogram((*climateArray)[startOffset + i]);
    }
    tfEditor->getQHistogram()->updateHistogram();
} 

void GCRMMesh::uploadCellGrids()
{
    ConfigManagerRef config = ConfigManager::getInstance();

    __uploadCellGrids( GetFloatArray("grid_center_lat" ),
                       GetFloatArray("grid_center_lon"),
                       GetIntArray("corner_edges"),
                       GetIntArray("corner_cells"),
                       GetIntArray("edge_cells"),
                       GetIntArray("edge_corners"),
                       GetIntArray("cell_corners"),
                       config->m_bCpuRender);
}

void GCRMMesh::__uploadCellGrids(
    HostFloatArrayRef  _h_gcrm_center_lat,	//hex center to latitude  not corner to latitude
    HostFloatArrayRef  _h_gcrm_center_lon,	//hex center to longitude not corner to longitude
    HostIntArrayRef	   _h_gcrm_corner_edges,//hex corner to hex edges
    HostIntArrayRef	   _h_gcrm_corner_cells,//hex corner to hex cells
    HostIntArrayRef	   _h_gcrm_edge_cells,  //hex edge to hexagons
    HostIntArrayRef	   _h_gcrm_edge_corners,//hex edge to hexagons corners
    HostIntArrayRef	   _h_gcrm_cell_corners,//hex cell to hexagons corners
    bool bToCPU)
{
    if (bToCPU)
    {
        GCRMConnectivity::h_gcrm_triangle_corner_lat	= _h_gcrm_center_lat ;
        GCRMConnectivity::h_gcrm_triangle_corner_lon	= _h_gcrm_center_lon ;
        GCRMConnectivity::h_gcrm_corner_edges	= _h_gcrm_corner_edges	;
        GCRMConnectivity::h_gcrm_corner_cells	= _h_gcrm_corner_cells	;
        GCRMConnectivity::h_gcrm_edge_cells	    = _h_gcrm_edge_cells	;
        GCRMConnectivity::h_gcrm_edge_corners	= _h_gcrm_edge_corners	;
        GCRMConnectivity::h_gcrm_cell_corners   = _h_gcrm_cell_corners	;

    }else{
        //////GPU data array//////////
        GCRMConnectivity::h_gcrm_triangle_corner_lat	= _h_gcrm_center_lat ;
        GCRMConnectivity::h_gcrm_triangle_corner_lon	= _h_gcrm_center_lon ;
        GCRMConnectivity::h_gcrm_corner_edges	= _h_gcrm_corner_edges	;
        GCRMConnectivity::h_gcrm_corner_cells	= _h_gcrm_corner_cells	;
        GCRMConnectivity::h_gcrm_edge_cells	    = _h_gcrm_edge_cells	;
        GCRMConnectivity::h_gcrm_edge_corners	= _h_gcrm_edge_corners	;
        GCRMConnectivity::h_gcrm_cell_corners   = _h_gcrm_cell_corners	;

        checkCudaErrors(cudaSetDevice(MAIN_GPU_ID));
        m_pDevTriangle_corner_lat1	= DevFloatArrayRef(new DevFloatArrayType(*_h_gcrm_center_lat));
        m_pDevTriangle_corner_lon1	= DevFloatArrayRef(new DevFloatArrayType(*_h_gcrm_center_lon));
        m_pDevCorner_edges1			= DevIntArrayRef(new DevIntArrayType(*_h_gcrm_corner_edges));
        m_pDevCorner_cells1			= DevIntArrayRef(new DevIntArrayType(*_h_gcrm_corner_cells));
        m_pDevEdge_cells1			= DevIntArrayRef(new DevIntArrayType(*_h_gcrm_edge_cells));
        m_pDevEdge_corners1			= DevIntArrayRef(new DevIntArrayType(*_h_gcrm_edge_corners));
        m_pDevCell_corners1			= DevIntArrayRef(new DevIntArrayType(*_h_gcrm_cell_corners));
        gcrm_uploadCellGrids(
            *m_pDevTriangle_corner_lat1	,
            *m_pDevTriangle_corner_lon1	,
            *m_pDevCorner_edges1		,
            *m_pDevCorner_cells1		,
            *m_pDevEdge_cells1			,
            *m_pDevEdge_corners1		,
            *m_pDevCell_corners1		);
        int nDevice=-1; checkCudaErrors(cudaGetDeviceCount(&nDevice));
        GPUVolumeRender* pGPUVolRender = dynamic_cast<GPUVolumeRender*>(m_volumeRender);
        if (pGPUVolRender && pGPUVolRender->UseMultipleGPU() && nDevice>1){
            checkCudaErrors(cudaSetDevice(abs(MAIN_GPU_ID-1)));//ad hoc to choose other GPU(since I have 2 GPUs)
            m_pDevTriangle_corner_lat2	= DevFloatArrayRef(new DevFloatArrayType(*_h_gcrm_center_lat));
            m_pDevTriangle_corner_lon2	= DevFloatArrayRef(new DevFloatArrayType(*_h_gcrm_center_lon));
            m_pDevCorner_edges2			= DevIntArrayRef(new DevIntArrayType(*_h_gcrm_corner_edges));
            m_pDevCorner_cells2			= DevIntArrayRef(new DevIntArrayType(*_h_gcrm_corner_cells));
            m_pDevEdge_cells2			= DevIntArrayRef(new DevIntArrayType(*_h_gcrm_edge_cells));
            m_pDevEdge_corners2			= DevIntArrayRef(new DevIntArrayType(*_h_gcrm_edge_corners));
            m_pDevCell_corners2			= DevIntArrayRef(new DevIntArrayType(*_h_gcrm_cell_corners));
            gcrm_uploadCellGrids(
                *m_pDevTriangle_corner_lat2	,
                *m_pDevTriangle_corner_lon2	,
                *m_pDevCorner_edges2		,
                *m_pDevCorner_cells2		,
                *m_pDevEdge_cells2			,
                *m_pDevEdge_corners2		,
                *m_pDevCell_corners2		);
            checkCudaErrors(cudaSetDevice(MAIN_GPU_ID));
        }
    }
}
void GCRMMesh::uploadClimateValue()
{
    ConfigManagerRef config = ConfigManager::getInstance();
    __uploadClimateValue(GetClimateVariableName(), m_curLocalTimeStep,
                         m_curLocalTimeStep, config->m_bCpuRender);
}

void GCRMMesh::__uploadClimateValue(const QString& climateValName,// HostFloatArrayPtr _h_climateValsRef,
                                    int timeStart, int timeEnd, bool bToCPU )
{
    //hijack climate value
    //HijackClimateData(_h_climateValsRef);
    //h_gcrm_ClimateVals = _h_climateValsRef	;
    //extract sub-array of _h_climateValsRef based on the timeStart and timeEnd
    HostFloatArrayRef _h_climateValsRef = GetFloatArray(climateValName);
    int dim1Size = _h_climateValsRef->m_dimsizes[0];
    int dim2Size = _h_climateValsRef->m_dimsizes[1];
    int dim3Size = _h_climateValsRef->m_dimsizes[2];
    if ((timeEnd-timeStart+1)>dim1Size){
        GLError::ErrorMessage(string(__func__)+string("(): Not enough time varying data loaded.\n\
                                      You're asking too much time slices"));
    }
    HostFloatArrayRef curTimeSliceData =
        HostFloatArrayRef(new HostFloatArrayType(HostFloatArrayType::DIM3,timeEnd-timeStart+1, dim2Size, dim3Size));
    HostFloatArrayType::value_type *pDst =  curTimeSliceData->m_vars.data();
    HostFloatArrayType::value_type *pScr = _h_climateValsRef->m_vars.data();
    int nElmnt = (timeEnd-timeStart+1)*dim2Size*dim3Size;
    memcpy(pDst, &(pScr[timeStart*dim2Size*dim3Size]), sizeof(HostFloatArrayType::value_type)*nElmnt);
    GCRMConnectivity::h_gcrm_ClimateVals = curTimeSliceData;

    if (!bToCPU)
    {//Using GPU rendering. upload CPU data to GPU.
        checkCudaErrors(cudaSetDevice(MAIN_GPU_ID));
        m_pDevClimateVals1 = DevFloatArrayRef(new DevFloatArrayType(*GCRMConnectivity::h_gcrm_ClimateVals));
        gcrm_uploadClimateValue(*m_pDevClimateVals1);
        int deviceID=-1; checkCudaErrors(cudaGetDevice(&deviceID));
        GPUVolumeRender* pGPUVolRender = dynamic_cast<GPUVolumeRender*>(m_volumeRender);
        if (pGPUVolRender && pGPUVolRender->UseMultipleGPU())
        {
            checkCudaErrors(cudaSetDevice(abs(MAIN_GPU_ID-1)));
            m_pDevClimateVals2 = DevFloatArrayRef(new DevFloatArrayType(*GCRMConnectivity::h_gcrm_ClimateVals));
            gcrm_uploadClimateValue(*m_pDevClimateVals2);
            checkCudaErrors(cudaSetDevice(MAIN_GPU_ID));
        }
    }

    printf("END of GCRMMesh::uploadClimateValue() \n");
}

//--------------For debugging purposes------------------------
void GCRMMesh::HijackClimateData( HostFloatArrayRef & _h_climateValsRef )
{
    HostFloatArrayRef varHexCenter = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lat"];
    int nCorners = varHexCenter->m_dimsizes[0];
    int nLayers	 = _h_climateValsRef->m_dimsizes[2];
    int dim2_size	=	_h_climateValsRef->m_dimsizes[1];//CLIMATE_VALS_DIMSIZES(1);
    int dim3_size	=	_h_climateValsRef->m_dimsizes[2];//CLIMATE_VALS_DIMSIZES(2);
    printf("#MaxLayers=%d\n",nLayers);
    HostIntArrayRef		varCornerCells = (*m_netmngr)[m_gridSimpleFileName]->getIntArray()["corner_cells"];
    HostFloatArrayRef	varCenterLat = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lat"];
    HostFloatArrayRef	varCenterLon = (*m_netmngr)[m_gridSimpleFileName]->getFloatArray()["grid_center_lon"];
    float R = GLOBLE_RADIUS;
    const float delLayer = GLOBLE_RADIUS*0.1f/(float)nLayers;
    int time = 0;
    for (int iCorner = 0 ; iCorner < nCorners ; ++iCorner)
    {
        //for (int i=0 ; i < 3 ; ++i)
        //{//get three centers of the neighborhood hexagon cells.
        //which are the vertex of the triangle we are about to draw later.
        //int	  iTriangleCorner = (*varCornerCells)[iCorner * 3+i];
        float iLat = (*varCenterLat)[iCorner];
        float iLon = (*varCenterLon)[iCorner];
        float sin_iLat = sin(iLat);
        float sin_iLon = sin(iLon);
        float cos_iLat = cos(iLat);
        float cos_iLon = cos(iLon);
        for (int iLayer=0 ; iLayer < nLayers ; ++iLayer)
        {
            //float val = computeScalarCenterToPole(iCorner, iLayer, delLayer, sin_iLat, sin_iLon, cos_iLat, cos_iLon);
            float val = computeScalar2(iCorner, iLayer, delLayer, iLat, sin_iLat, sin_iLon, cos_iLat, cos_iLon);
            (*_h_climateValsRef)(HostFloatArrayType::DIM3, time, iCorner, iLayer) = val;
        }
    }
    _h_climateValsRef->normalize();
}

float GCRMMesh::computeScalar2(const int iCorner,const int iLayer,const float delLayer, const float theta,
    const float sin_iLat,const float sin_iLon,const float cos_iLat,const float cos_iLon)
{
    //for each layer
    float R = GLOBLE_RADIUS - delLayer*iLayer;// Radius of top triangle in current cell
#define  Feq  60.0f
#define  A 0.033f
    float val = (A*sin(Feq*(PI*0.5f - theta)) + 1.0f)*R;
#undef Feq
#undef A
    return val;
}
float GCRMMesh::computeScalarCenterToPole(const int iCorner,const int iLayer,const float delLayer
    ,const float sin_iLat,const float sin_iLon,const float cos_iLat,const float cos_iLon)
{
    //for each layer
    float R = GLOBLE_RADIUS - delLayer*iLayer;// Radius of top triangle in current cell
    float3 position = make_float3(R*cos_iLat*sin_iLon, R*sin_iLat, R*cos_iLat*cos_iLon );
    //x[i]	= R*cos(iLat)*sin(iLon);
    //y[i]	= R*sin(iLat);
    //z[i]	= R*cos(iLat)*cos(iLon);
    float dist2NorthPole = length(position - make_float3(0.0f,GLOBLE_RADIUS,0.0f)) ;
    float dist2SouthPole = length(position - make_float3(0.0f,-GLOBLE_RADIUS,0.0f));
    float val = min(dist2NorthPole, dist2SouthPole)*0.7072;
    
    return val;//(*varClimate)[time*dim2_size*dim3_size + iCell*dim3_size + intface];
}
*/

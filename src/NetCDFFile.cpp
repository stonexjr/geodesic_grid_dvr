#include "NetCDFFile.h"

string NetCDFFile::dummyName="dummyFile";
NetCDFFile NetCDFFile::dummyFile("dummyFile");

NetCDFFile::NetCDFFile(void)
{
}

NetCDFFile::NetCDFFile( const string& filepath )
{
    if (filepath=="dummyFile")
        return;

    const char* c_str = filepath.data();
    m_pNetcdf = NetCDFRef(new NetCDF(c_str));
}

NetCDFFile::~NetCDFFile(void)
{
}

//var_name: the text name appeared in netcdf.
//var_id: unique identifier for this array.
HostFloatArrayRef NetCDFFile::addFloatArray( const string& var_name,
    const string& var_id, 
    int dim1limit/*=-1*/, int dim2limit/*=-1*/, int dim3limit/*=-1*/ )
{
    const char* c_str = var_name.data();
    NetCDF_var* var_info = m_pNetcdf->get_varinfo(c_str);
    assert(var_info->rh_type == NC_FLOAT);
    int dim1=1, dim2=1, dim3=1;
    int ndim = var_info->rh_ndims;
    switch(ndim)
    {
        case 1:
            dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
            break;
        case 2:
            dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
            dim2 = (dim2limit == -1) ? var_info->dim[1]->length : dim2limit;
            break;
        case 3:
            dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
            dim2 = (dim2limit == -1) ? var_info->dim[1]->length : dim2limit;
            dim3 = (dim3limit == -1) ? var_info->dim[2]->length : dim3limit;
    }
    HostFloatArrayRef array = HostFloatArrayRef(new HostFloatArrayType((HostFloatArrayType::dims)ndim, dim1,dim2,dim3));//boost::shared_ptr<FloatArray>(new FloatArray(ndim, dim1,dim2,dim3));
    m_floatArrays[var_id] = array;

    return array;
}

HostDoubleArrayRef NetCDFFile::addDoubleArray( const string& var_name,
    const string& var_id,
    int dim1limit/*=-1*/, int dim2limit/*=-1*/, int dim3limit/*=-1*/ )
{
    const char* c_str = var_name.data();//qPrintable(var_name);
    NetCDF_var* var_info = m_pNetcdf->get_varinfo(c_str);
    assert(var_info->rh_type == NC_DOUBLE);
    int dim1=1, dim2=1, dim3=1;
    int ndim = var_info->rh_ndims;
    switch(ndim)
    {
        case 1:
            if (strcmp(var_info->dim[0]->dim_name,"time")==0)
            {
                dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
            }else{
                dim1 = var_info->dim[0]->length;
            }
            break;
        case 2:
            dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
            dim2 = (dim2limit == -1) ? var_info->dim[1]->length : dim2limit;
            break;
        case 3:
            dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
            dim2 = (dim2limit == -1) ? var_info->dim[1]->length : dim2limit;
            dim3 = (dim3limit == -1) ? var_info->dim[2]->length : dim3limit;
    }
    HostDoubleArrayRef array = HostDoubleArrayRef(new HostDoubleArrayType((HostDoubleArrayType::dims)ndim, dim1,dim2,dim3));//boost::shared_ptr<FloatArray>(new FloatArray(ndim, dim1,dim2,dim3));
    m_doubleArrays[var_id] = array;

    return array;
}

HostIntArrayRef NetCDFFile::addIntArray( const string& var_name,
    const string& var_id,
    int dim1limit/*=-1*/, int dim2limit/*=-1*/, int dim3limit/*=-1*/  )
{
    const char* c_str = var_name.data();//qPrintable(var_name);
    NetCDF_var* var_info = m_pNetcdf->get_varinfo(c_str);
    assert(var_info->rh_type == NC_INT);
    int dim1=1, dim2=1, dim3=1;
    int ndim = var_info->rh_ndims;
    switch(ndim)
    {
    case 1:
        dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
        break;
    case 2:
        dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
        dim2 = (dim2limit == -1) ? var_info->dim[1]->length : dim2limit;
        break;
    case 3:
        dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
        dim2 = (dim2limit == -1) ? var_info->dim[1]->length : dim2limit;
        dim3 = (dim3limit == -1) ? var_info->dim[2]->length : dim3limit;
    }
    HostIntArrayRef array = HostIntArrayRef(new HostIntArrayType((HostIntArrayType::dims)ndim, dim1,dim2,dim3));
    m_intArrays[var_id] = array;
    return array;
}

HostByteArrayRef NetCDFFile::addByteArray( const string& var_name,
    const string& var_id,
    int dim1limit/*=-1*/, int dim2limit/*=-1*/, int dim3limit/*=-1*/  )
{
    const char* c_str = var_name.data();//qPrintable(var_name);
    NetCDF_var* var_info = m_pNetcdf->get_varinfo(c_str);
    assert(var_info->rh_type == NC_BYTE);
    int dim1=1, dim2=1, dim3=1;
    int ndim = var_info->rh_ndims;
    switch(ndim)
    {
    case 1:
        dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
        break;
    case 2:
        dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
        dim2 = (dim2limit == -1) ? var_info->dim[1]->length : dim2limit;
        break;
    case 3:
        dim1 = (dim1limit == -1) ? var_info->dim[0]->length : dim1limit;
        dim2 = (dim2limit == -1) ? var_info->dim[1]->length : dim2limit;
        dim3 = (dim3limit == -1) ? var_info->dim[2]->length : dim3limit;
    }
    HostByteArrayRef array = HostByteArrayRef(new HostByteArrayType((HostByteArrayType::dims)ndim, dim1,dim2,dim3));//boost::shared_ptr<IntArray>(new IntArray(ndim, dim1,dim2,dim3));
    m_byteArrays[var_id] = array;
    return array;
}

vector<string> NetCDFFile::GetVarNames( int* var, int dim )
{
    return m_pNetcdf->GetVarNames(var, dim);
}

bool NetCDFFile::LoadVarData( const string& var_name, const string& var_id,
    int time_in, int time_count, int start_var, int fraction, bool bFullSize /*= false*/ )
{
    char* pBuffer=NULL;
    if(m_floatArrays.find(var_id) != m_floatArrays.end()){

        HostFloatArrayRef array = m_floatArrays[var_id];
        pBuffer = reinterpret_cast<char*>(&(array->m_vars[0]));

    }else if (m_intArrays.find(var_id) != m_intArrays.end()){

        HostIntArrayRef array = m_intArrays[var_id];
        pBuffer = reinterpret_cast<char*>(&(array->m_vars[0]));

    }else if (m_doubleArrays.find(var_id) != m_doubleArrays.end()){

        HostDoubleArrayRef array = m_doubleArrays[var_id];
        pBuffer = reinterpret_cast<char*>(&(array->m_vars[0]));
        array->fill(0.0);
    }

    if (pBuffer)
    {
        const char* c_str = var_name.data();//qPrintable(var_name);
        return m_pNetcdf->get_vardata( c_str, time_in, time_count, start_var, fraction, &pBuffer, bFullSize);
        //return m_pNetcdf->get_vardata( c_str, time_in, time_out, 0, 1, &pBuffer, true);
    }

    return false;
}

//void NetCDFFile::insertFloatArray( const string& var_name, boost::shared_ptr<FloatArray> farray )
void NetCDFFile::insertFloatArray(const string& var_name, HostFloatArrayRef farray)
{
    if (m_floatArrays.find(var_name) == m_floatArrays.end())
    {
        m_floatArrays[var_name] = farray;
    }
}

//void NetCDFFile::insertIntArray( const string& var_name, boost::shared_ptr<IntArray> iarray )
void NetCDFFile::insertIntArray( const string& var_name, HostIntArrayRef iarray)
{
    if (m_intArrays.find(var_name) == m_intArrays.end())
    {
        m_intArrays[var_name] = iarray;
    }
}

void NetCDFFile::removeFloatArray( const string& var_name )
{
    m_floatArrays.erase(var_name);
}

void NetCDFFile::removeDoubleArray( const string& var_name )
{
    m_doubleArrays.erase(var_name);
}

void NetCDFFile::removeIntArray( const string& var_name )
{
    m_intArrays.erase(var_name);
}

void NetCDFFile::removeByteArray( const string& var_name )
{
    m_byteArrays.erase(var_name);
}

NetCDF_var* NetCDFFile::GetVarInfo( const string& var_name )
{
    const char* name = var_name.data();//qPrintable(var_name);
    return m_pNetcdf->get_varinfo(name);
}

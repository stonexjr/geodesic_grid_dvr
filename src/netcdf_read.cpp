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

#include "netcdf_read.h"
#include <time.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include <sys/types.h>
#include <sys/timeb.h>

// Handle errors by printing an error message and exiting with a
// non-zero status.
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); return 2;}

NetCDF::NetCDF(const char *filename)
{
    m_init = false;
    m_Nd_var_num = 0;
    strcpy(m_filename, filename);

    load_header();

    m_tacfnname = "";
    m_tacfn = NULL;
    srand((unsigned)time( NULL ));
}

NetCDF::~NetCDF()
{
    for(int i=0; i < m_nvars; i++) {
        if (m_var_arr[i])
            delete m_var_arr[i];
    }

    if (m_init)
        nc_close(m_ncid);
}

char** NetCDF::get_varnames(int *num, int dim)
{
    char** names = NULL;
    *num = 0;
    int j = 0;
    if( m_init) {
        names = new char*[m_nvars];
        for(int i=0; i < m_nvars; i++) {
            if (m_var_arr[i]->rh_ndims == dim) {
                names[j] = new char[NC_MAX_NAME];
                strcpy(names[j], m_var_arr[i]->var_name);
                j++;
            }
        }
        *num = j;
    }
    return names;
}

vector<string> NetCDF::GetVarNames(int *num, int dim)
{
    char** names = NULL;
    vector<string> strlist;
    *num = 0;
    int j = 0;
    if( m_init) {
        names = new char*[m_nvars];
        for(int i=0; i < m_nvars; i++) {
            if (m_var_arr[i]->rh_ndims == dim) {
                names[j] = new char[NC_MAX_NAME];
                strcpy(names[j], m_var_arr[i]->var_name);
                strlist.push_back(string(names[j]));
                j++;
            }
        }
        *num = j;
    }
    return strlist;
}

int NetCDF::load_header()
{
    int status, ngatts, unlimdimid;

    status = nc_open(m_filename, NC_NOWRITE, &m_ncid);
    if (status != NC_NOERR) ERR(status);

    //find out what is in it
    status = nc_inq(m_ncid, &m_ndims, &m_nvars, &ngatts, &unlimdimid);
    if (status != NC_NOERR) ERR(status);

    char* var_name = new char[NC_MAX_NAME];

    for(int i=0; i < m_ndims; i++) {
        char* dim_name = new char[NC_MAX_NAME];
        size_t lengthp;
        // get dimension names, lengths
        status = nc_inq_dim(m_ncid, i, dim_name, &lengthp);
        if (status != NC_NOERR) ERR(status);
        m_dim_name_arr[i] = dim_name;
        m_dim_length_arr[i] = lengthp;
    }

    for(int i=0; i < m_nvars; i++) {
        //nc_inq_var
        m_var_arr.push_back(new NetCDF_var());

        m_var_arr[i]->order = -1;
        m_var_arr[i]->axis = '\0';

        //get variable names, types, shapes
        status = nc_inq_var (m_ncid, i,
            m_var_arr[i]->var_name,
            &(m_var_arr[i]->rh_type),
            &(m_var_arr[i]->rh_ndims),
            m_var_arr[i]->rh_dimids,
            &(m_var_arr[i]->rh_natts));
        if (status != NC_NOERR) ERR(status);

        m_Nd_var[m_var_arr[i]->var_name] = i;
        m_Nd_var_num++;

        if ((status = nc_inq_varid(m_ncid, m_var_arr[i]->var_name, &(m_var_arr[i]->temp_varid))))
            ERR(status);

        m_var_arr[i]->total_size = 1;
        m_var_arr[i]->time_dep = false;
        m_var_arr[i]->count = new size_t[m_var_arr[i]->rh_ndims];
        m_var_arr[i]->start = new size_t[m_var_arr[i]->rh_ndims];
        for (int j=0; j < m_var_arr[i]->rh_ndims; j++) {
            m_var_arr[i]->dim[j] = new NetCFD_dim;
            m_var_arr[i]->dim[j]->length = (int) m_dim_length_arr[m_var_arr[i]->rh_dimids[j]];
            strcpy(m_var_arr[i]->dim[j]->dim_name, m_dim_name_arr[m_var_arr[i]->rh_dimids[j]]);
            if(strcmp(m_var_arr[i]->dim[j]->dim_name,"time")!=0)
                m_var_arr[i]->total_size = m_var_arr[i]->total_size *  m_var_arr[i]->dim[j]->length;
            else
                m_var_arr[i]->time_dep = true;
            m_var_arr[i]->dim[j]->query_start = 0;
            m_var_arr[i]->dim[j]->query_count = m_var_arr[i]->dim[j]->length;
        }
        if(m_var_arr[i]->total_size == 1)
            m_var_arr[i]->total_size = 0;

        //process some attributes
        /* find out how much space is needed for attribute values */
        char* axis;
        size_t  vr_len;
        status = nc_inq_attlen (m_ncid, m_var_arr[i]->temp_varid, "cartesian_axis", &vr_len);
        if (status == NC_NOERR) {
            /* allocate required space before retrieving values */
            axis = new char[vr_len + 1];  /* + 1 for trailing null */

            /* get attribute values */
            status = nc_get_att_text(m_ncid, m_var_arr[i]->temp_varid, "cartesian_axis", axis);
            if (status != NC_NOERR)
                ERR(status);
            axis[vr_len] = '\0';       /* null terminate */
            if (strcmp(axis, "X")==0){
                m_var_arr[i]->axis = 'X';
            }
            else if (strcmp(axis, "Y")==0){
                m_var_arr[i]->axis = 'Y';
            }
            else if (strcmp(axis, "Z")==0){
                m_var_arr[i]->axis = 'Z';
            }
            delete axis;
        }
    }

    m_init = true;

    return status;
}

size_t NetCDF::get_var_size(char* name)
{
    size_t current_size = 1;
    int i = m_Nd_var[name];
    for (int j=0; j < m_var_arr[i]->rh_ndims; j++) {
        //remember count[0] should be 1 ( for 1 time step)
        if (strcmp(m_dim_name_arr[m_var_arr[i]->rh_dimids[j]], "time")!=0)
            current_size = current_size * m_var_arr[i]->dim[j]->query_count;
    }
    return current_size;
}

NetCDF_var* NetCDF::get_varinfo(const char* name, Filter* fltr)
{
    //check if key "name" contained in the map "m_Nd_var"
    if (m_Nd_var.find(name) == m_Nd_var.end())
    {
        return NULL;
    }

    int idx = m_Nd_var[name];
    return get_varinfo_int(idx, fltr);
}

NetCDF_var* NetCDF::get_varinfo_int(int index, Filter* fltr)
{
    NetCDF_var* var = m_var_arr[index];
    if(fltr){
        if(strcmp(fltr->m_include_str, "\0") != 0)
            if(strcmp(fltr->m_include_str, var->var_name) != 0)
                return NULL;
        if(strcmp(fltr->m_exclude_str, "\0") != 0)
            if(strstr(fltr->m_exclude_str, var->var_name) == 0)
                return NULL;
        if(fltr->m_dim != var->rh_ndims && fltr->m_dim != ANY_DIM)
            return NULL;
    }
    return var;
}

size_t NetCDF::get_num_timesteps(char* name)
{
    NetCDF_var *var =  m_var_arr[m_Nd_var[name]];
    int rh_ndims = var->rh_ndims;
    int* rh_dimids = var->rh_dimids;
    for (int j=0; j < rh_ndims; j++) {
        //remember count[0] should be 1 ( for 1 time step)
        if (strcmp(m_dim_name_arr[rh_dimids[j]], "time")==0){
            return m_dim_length_arr[rh_dimids[j]];
        }
    }
    return 0;
}

// caller needs to pass fully allocated buffer to hold the data
int NetCDF::get_vardata(const char* index,
                        int time_in,
                        int time_count,
                        int start_var,
                        int fraction,
                        /* out */char** data,
                        bool bFullSize)

{
    return get_vardata_impl(get_varinfo(index),
        time_in,
        time_count,
        start_var,
        fraction,
        data,
        bFullSize);
}

// caller needs to pass fully allocated buffer to hold the data

int NetCDF::get_vardata_impl(NetCDF_var *var,
    int time_in,
    int count_in,
    int start_var,
    int fraction,
    /* out */char** data,
    bool bFullSize)

{
    if (!var)
        return 0;
    char* var_name = var->var_name;
    nc_type rh_type = var->rh_type;
    int rh_ndims = var->rh_ndims;
    int* rh_dimids = var->rh_dimids;
    int temp_varid = var->temp_varid;
    size_t* start = var->start;
    size_t* count = var->count;
    int status;

    size_t total_size = 1;
    for (int j = 0; j < rh_ndims; j++) {
        //remember count[0] should be 1 ( for 1 time step)
        /*
        if (strcmp(m_dim_name_arr[rh_dimids[j]], "time")==0) {
        start[j] = time_in;
        count[j] = count_in;
        }
        else {
        count[j] = m_dim_length_arr[rh_dimids[j]]/fraction;
        start[j] = start_var;
        }
        */
        string cur_dim_name(var->dim[j]->dim_name);
        std::transform(cur_dim_name.begin(), cur_dim_name.end(),
            cur_dim_name.begin(), tolower);

        //if(strcmp(var->dim[j]->dim_name,"time")==0){
        if (cur_dim_name == "time")
        {
            start[j] = time_in;
            count[j] = count_in;
        }
        else {
            if (!bFullSize){
                if (count[j] = var->dim[j]->query_count > fraction)
                    count[j] = var->dim[j]->query_count / fraction;
                else
                    count[j] = var->dim[j]->query_count;
                start[j] = var->dim[j]->query_start;
            }
            else{
                count[j] = var->dim[j]->length;
                start[j] = 0;
            }
        }
        total_size = total_size * count[j];
    }

    if (rh_ndims == 0) total_size = 0;

    switch (rh_type){
    case NC_FLOAT:
        if ((status = nc_get_vara_float(m_ncid, temp_varid, start,
            count, reinterpret_cast<float *>(*data))))
            ERR(status);
        break;
    case NC_DOUBLE:
        if ((status = nc_get_vara_double(m_ncid, temp_varid, start,
            count, reinterpret_cast<double *>(*data))))
            ERR(status);
        break;
    case NC_INT:
        if ((status = nc_get_vara_int(m_ncid, temp_varid, start,
            count, reinterpret_cast<int *>(*data))))
            ERR(status);
        break;
    case NC_SHORT:
        if ((status = nc_get_vara_short(m_ncid, temp_varid, start,
            count, reinterpret_cast<short *>(*data))))
            ERR(status);
        break;
    case NC_CHAR:
    case NC_BYTE:
        if ((status = nc_get_vara_uchar(m_ncid, temp_varid, start,
            count, reinterpret_cast<unsigned char*>(*data))))
            ERR(status);
    default:
        break;
    }
    return status;
}

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

#ifndef _NETCTDFILE_H
#define  _NETCTDFILE_H

#include "netcdf_read.h"
#include "HostArrayTemplate.h"
#include <map>
#include <memory>

class NetCDFFile
{
public:

    NetCDFFile(void);
    NetCDFFile(const string& filepath);
    ~NetCDFFile(void);

    //************************************************************************
    // Reserve and Register a  floating point array without actually load the data
    // Parameter: const string & var_name : The string name id of this array.
    // If the size of the dataset is too huge, it might help to load partial data by
    // setting the following dimension size limitation.
    // Parameter: int dim1limit : The limitation on the number of elements in dimension 1
    // Parameter: int dim2limit : The limitation on the number of elements in dimension 2
    // Parameter: int dim3limit : The limitation on the number of elements in dimension 3
    //************************************************************************
    HostFloatArrayRef addFloatArray(const string& var_name, const string& var_id,  int dim1limit=-1, int dim2limit=-1, int dim3limit=-1);

        map<string, HostFloatArrayRef > getFloatArray(){return m_floatArrays;}

    HostDoubleArrayRef addDoubleArray(const string& var_name,  const string& var_id,  int dim1limit=-1, int dim2limit=-1, int dim3limit=-1);

        map<string, HostDoubleArrayRef > getDoubleArray(){return m_doubleArrays;}
    
    //************************************************************************
    // Reserve and Register a  Integer point array without actually load the data
    // Parameter: const string & var_name : The string name id of this array.
    // If the size of the dataset is too huge, it might help to load partial data by
    // setting the following dimension size limitation.
    // Parameter: int dim1limit : The limitation on the number of elements in dimension 1
    // Parameter: int dim2limit : The limitation on the number of elements in dimension 2
    // Parameter: int dim3limit : The limitation on the number of elements in dimension 3
    //************************************************************************
    HostIntArrayRef addIntArray(const string& var_name, const string& var_id, int dim1limit=-1, int dim2limit=-1, int dim3limit=-1);

        map<string, HostIntArrayRef > getIntArray(){return m_intArrays;}

    HostByteArrayRef addByteArray(const string& var_name, const string& var_id, int dim1limit=-1, int dim2limit=-1, int dim3limit=-1);
        map<string, HostByteArrayRef > getByteArray(){return m_byteArrays;}
    // Method:    loadVarData
    // FullName:  NetCDFFile::loadVarData
    // Access:    public
    // Returns:   true if loaded successfully.
    bool	LoadVarData(const string& var_name, const string& var_id,
                        int time_in, int time_count, int start_var, 
                        int fraction, bool bFullSize = false);

    void	insertFloatArray(const string& var_name, HostFloatArrayRef farray);
    void	insertIntArray(const string& var_name, HostIntArrayRef iarray);
    // 	void	insertFloatArray(const string& var_name, boost::shared_ptr<FloatArray> farray);
    // 	void	insertIntArray(const string& var_name, boost::shared_ptr<IntArray> iarray);
    NetCDF_var* GetVarInfo(const string& var_name);
    void	removeDoubleArray(const string& var_name);
    void	removeFloatArray(const string& var_name);
    void	removeIntArray(const string& var_name);
    void    removeByteArray(const string& var_name);
    vector<string> GetVarNames(int* var, int dim);

    static string dummyName;
    static NetCDFFile dummyFile;
protected:
        NetCDFRef m_pNetcdf;
        map<string, HostDoubleArrayRef >	m_doubleArrays;
        map<string, HostFloatArrayRef >	m_floatArrays;
        map<string, HostIntArrayRef >	m_intArrays;
        map<string, HostByteArrayRef >	m_byteArrays;
};
typedef	std::shared_ptr<NetCDFFile> NetCDFFileRef;

#endif

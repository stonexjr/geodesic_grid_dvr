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

#include "NetCDFManager.h"
#include "Dir.h"

NetCDFManager::NetCDFManager()
{
}

NetCDFManager::~NetCDFManager()
{
}

NetCDFFileRef NetCDFManager::addNetCDF( const string& fileName )
{
    if (fileName == NetCDFFile::dummyName)
    {
        m_netcdfFiles[fileName] = NetCDFFileRef(new NetCDFFile(NetCDFFile::dummyName));
    }
    string nativeFileName = Dir::toNativeSeparators(fileName);
    //QByteArray ba = nativeFileName.toLatin1();
    //char* c_str = ba.data();
    string sptr = Dir::separator();
    //string index = nativeFileName.section(sptr,-1,-1,string::SectionSkipEmpty);
    string index = Dir::basename(nativeFileName);
    m_netcdfFiles[index] = NetCDFFileRef(new NetCDFFile(nativeFileName));
    return m_netcdfFiles[index];
}

void NetCDFManager::addNetCDF(const string& SimplefileName,const NetCDFFileRef& file )
{
    m_netcdfFiles[SimplefileName] = file;
}
NetCDFFileRef& NetCDFManager::operator[]( const string& index )
{
    return m_netcdfFiles[index];
}
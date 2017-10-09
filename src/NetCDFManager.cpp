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
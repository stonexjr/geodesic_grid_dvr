#ifndef NETCDFMANAGER_H
#define NETCDFMANAGER_H

#include <map>

#include "NetCDFFile.h"

class NetCDFManager 
{

	map<string, NetCDFFileRef > m_netcdfFiles;
public:
	NetCDFManager();
	~NetCDFManager();
	//fileName: file name with full path
	//return reference to the newly added netcdf file if
	//successful. or else return NULL.
	NetCDFFileRef  addNetCDF(const string& fileName );
	void		   addNetCDF(const string& SimplefileName,
							 const NetCDFFileRef& file );
	NetCDFFileRef&
		operator[](const string& index);

private:
};

typedef std::shared_ptr<NetCDFManager> NetCDFManagerRef;

#endif // NETCDFMANAGER_H
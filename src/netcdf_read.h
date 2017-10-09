#ifndef NETCDF_READ
#define NETCDF_READ

#include <string>
#include <netcdf.h>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <map>
#include <memory>

#define MAX_PATH1   512
#define SEARCH_DIM 	4  // we are only interested in data with SEARCH_DIM number of dimensions

using namespace std;

struct NetCFD_dim
{
	char dim_name[NC_MAX_NAME];
	int length;
	int query_start;
	int query_count;
};

struct NetCDF_var
{
	NetCDF_var(){}
	~NetCDF_var(){}

	char var_name[NC_MAX_NAME];
	nc_type rh_type;                  /* variable type */
	int rh_ndims;                      /* number of dims */
	int rh_dimids[NC_MAX_VAR_DIMS];    /* dimension ids */
	NetCFD_dim * dim[NC_MAX_VAR_DIMS];
	int rh_natts;                      /* number of attributes */
	int temp_varid;
	size_t* start;
	size_t* count;
	int total_size;
	bool time_dep;

	//order: only used if var is a dim, default value is -1 for all other vars
	int order;
	//axis only used for 1-dim vardims, default '\0'
	char axis;
};

#define ANY_DIM		0
struct Filter
{
	Filter()
	{
		m_exclude_str[0] = '\0';
		m_include_str[0] = '\0';
		m_dim = ANY_DIM;
	}

	int m_dim;
	char m_exclude_str[MAX_PATH1];
	char m_include_str[MAX_PATH1];
};

class NetCDF
{
public:
	NetCDF(const char* filename);
	~NetCDF();
	int load_header();
	int read_var(NetCDF_var *var);

	// returns number of variables and array of variable names
	char** get_varnames(/*out*/ int *num, int dim = SEARCH_DIM);
	vector<string> GetVarNames(int *num, int dim = SEARCH_DIM);
	size_t get_num_timesteps(char* name);
	NetCDF_var* get_varinfo(const char* name, Filter* fltr = NULL);
	NetCDF_var* get_varinfo_int(int index, Filter* fltr = NULL);

	// caller needs to pass fully allocated buffer to hold the data

	//wrapper on top next get_vardata_impl

	int get_vardata(const char* index,
						int time_in,
						int time_count,
						int start_var,
						int fraction,
						/* out */char** data,
						bool bFullSize = false);
	int get_vardata_int(int index,
						int time_in,
						int time_count,
						int start_var,
						int fraction,
						/* out */char** data,
						bool bFullSize = false);

	// caller needs to pass fully allocated buffer to hold the data

	int get_vardata_impl(NetCDF_var *var,
		int time_in,
		int time_out,
		int start_var,
		int fraction,
		/* out */char** data,
		bool bFullSize = false);

	int get_varindices();
	//size_t get_var_size(int index);
	size_t get_var_size(char* name);

	bool getMinMax(char* varname, float* min, float* max);
	int get_num_vars()
	{
		return m_nvars;
	}
	// return the map from index of
	//map<int,int> get_ind_map(int dim);
	int get_totalsize(const char* var_name, int* datasize);

	bool readtac(const char* filename, const char* varname, float* points,

					 int start, int end, /* spatial indices */

					 int total_timesteps, /* total num of timesteps in TAC file */

					 int timesteps /* timesteps needed */);

	bool savetimestep(const char* filename, const char* varname, int timestep, float* pdata, int sizeofvolume);

	bool createtacfile(const char* filename, const char* varname, int start, int end, int sizeofvolume);

	bool checktacfile(const char* filename, const char* varname, int tdim);

	bool resettachandle(const char* filename, const char* varname, int tdim);

public:
	int m_ncid;
	int m_ndims;							//number of all dimesions
	int m_nvars;
	char* m_dim_name_arr[NC_MAX_VAR_DIMS];				//dimensions length
	size_t m_dim_length_arr[NC_MAX_VAR_DIMS];			//dimensions name
	char m_filename[MAX_PATH1];			//netCDF filename (ext .nc)
	vector<NetCDF_var*> m_var_arr;
	map<string, int> m_Nd_var;
	int m_Nd_var_num;
	bool m_init;

	string m_tacfnname;
	FILE* m_tacfn;
};

typedef std::shared_ptr<NetCDF> NetCDFRef;

#endif //NETCDF_READ
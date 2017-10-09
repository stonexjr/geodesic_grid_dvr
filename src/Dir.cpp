#include "Dir.h"
#include <algorithm>


Dir::Dir()
{
}


Dir::~Dir()
{
}

string Dir::toNativeSeparators(const string& path)
{
    string ret = path;
    std::replace(ret.begin(), ret.end(), Dir::nonstdSptr[0], Dir::sptr[0]);
    return ret;
}

std::string Dir::basename(const string& path)
{
    string npath = path;
    //normalize npath based on the OS
    npath = Dir::toNativeSeparators(npath);
    size_t loc = npath.rfind(Dir::sptr);
    if (loc != string::npos){
        return npath.substr(loc+1);
    }else{
        return npath;
    }
}

#ifdef WIN32
string Dir::sptr="\\";
string Dir::nonstdSptr = "/";
#else
std::string Dir::sptr="/";
string Dir::nonstdSptr = "\";
#endif

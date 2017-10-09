#ifndef DIR_H
#define DIR_H
#include <string>
using namespace std;
class Dir
{
public:
    Dir();
    ~Dir();
    static string separator(){ return sptr; }
    static string toNativeSeparators(const string& path);
    static string basename(const string& path);
    static string sptr;
private:
    static string nonstdSptr;
};

#endif

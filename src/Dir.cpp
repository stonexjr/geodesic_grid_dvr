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
string Dir::nonstdSptr = "\\";
#endif

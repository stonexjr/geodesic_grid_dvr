#ifndef _HOST_ARRAY_TEMPLATE_H
#define _HOST_ARRAY_TEMPLATE_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <vector>
#include <memory>
#ifdef USE_CUDA
#include <helper_math.h>
#include "DevArrayTemplate.h"
#endif
#include <stdarg.h>

using namespace std;
//Multi-dimensional array
//define host/device vector dimension
// #define  DIM1	1 //1 dimensional
// #define  DIM2	2 //2 dimensional
// #define  DIM3	3 //3 dimensional
// #define  DIM4	4 //4 dimensional
template<typename T>
class HostArray
{
public:
    typedef enum{DIM1=1,DIM2=2,DIM3=3,DIM4=4,MAX_DIMS=3}dims;
    typedef	T value_type;
    int		m_ndim;	//number of dimensions of this array.
    //the size of each dimensions.
    //Eg. m_dimsizes[0]=zDim;m_dimsizes[1]=yDim;m_dimsizes[0]=xDim;
    vector<size_t>	m_dimsizes;//int		m_dimsizes[MAX_DIMS];//
    std::vector<T>  m_vars;
    typedef std::vector<T> vector_type;
    //int ndim can be DIM1, DIM2, DIM3 or DIM4
    HostArray(dims ndim, ... );//
    ~HostArray(void);//
    T&	operator [](int i){ return m_vars[i];}//
    T	operator [](int i) const{ return m_vars[i]; }//
    //returns a copy of array element by specifying:
    //n: the number array dimension;
    // ...: n number of indices for each dimension in reverse order
    // in which the index dimension that changes slowest is put as the first parameter
    // while the fastest changing dimensional index is assigned to the last parameter.
    // Eg. Calling array(3, idx_dim1, idx_dim2, idx_dim3) is equivalent to referencing
    // the data element as array[idx_dim1*dim2_size*dim3_size + idx_dim2*dim3_size + idx_dim3].
    // for GCRM data, you can specify the indices like array(3, idx_time, idx_cell, idx_layer).
    T operator () (int n,...) const;
    //returns a reference of array element by specifying:
    //n: the number array dimension;
    // ...: n number of index for each dimension in reverse order,
    // in which the index dimension that changes slowest is put as the first parameter
    // while the fastest changing dimensional index is assigned to the last parameter.
    T& operator () (int n,...);
    void normalize();//
    void normalize(T min_t, T max_t);
    void logNormalize();
    void getMinMax(T& min_t, T& max_t);
    void fill(T val);
#ifdef USE_CUDA
    HostArray<T>& copy_dev2host(const DevArray<T>& d_array);//
    HostArray<T>& copy_dev2host(const T* d_pointer, size_t nElmnts );//
#endif
    void resize(size_t size);
    void* data(){ return m_vars.data();}
        template<typename T2>
        friend ostream& operator << ( ostream& os, const HostArray<T2> &t);
        template<typename T2>
        friend istream& operator >> ( istream& os, const HostArray<T2> &t);
        template<typename T2>
        friend ostream& operator << ( ostream& os, HostArray<T2> &t);
        template<typename T2>
        friend istream& operator >> ( istream& os, HostArray<T2> &t);
};

template<typename T>
void HostArray<T>::fill( T val )
{
    size_t size = m_vars.size();
    for (size_t i=0; i < size; i++){
        m_vars[i] = val;
    }
}

template<typename T>
HostArray<T>::HostArray(dims ndim, ... )
    :m_vars(0)
{
    if (ndim==0)
        return;
    m_ndim = ndim;

    m_dimsizes.resize(ndim, 1);
    //m_dimsizes.fill(1);
    size_t size=1;
    va_list vl;
    va_start(vl, ndim);
    for (int i = 0; i < ndim ; i++)
    {
        m_dimsizes[i] = (int)va_arg(vl, int);
        size *= m_dimsizes[i]>= 1 ? m_dimsizes[i] : (m_dimsizes[i]=1);
    }
    va_end(vl);
    m_vars.resize(size);
}

template<typename T>
HostArray<T>::~HostArray(void){
    printf("~HostArray() Free %f MB CPU memory\n", (float)(m_vars.size()*sizeof(T))/1048576.0f);
    // 	if (m_vars)
    // 	{
    // 		delete[] m_vars;
    // 		m_vars = NULL;
    // 	}
}
template<typename T>
void HostArray<T>::normalize(){
    T min_t, max_t;
    getMinMax(min_t, max_t);
    float inv_scale = 1.0f/(float)(max_t - min_t);
    int size = m_vars.size();
    for (int i=0 ; i < size ; ++i)
    {
        m_vars[i] = (m_vars[i] - min_t)*inv_scale;
        m_vars[i] = m_vars[i]>0.0 ? m_vars[i] : 0.0;//max(m_vars[i], 0.0f);
        m_vars[i] = m_vars[i]<1.0 ? m_vars[i] : 1.0;//min(m_vars[i], 1.0f);
    }
}

template<typename T>
void HostArray<T>::normalize(T min_t, T max_t)
{

    float inv_scale = 1.0f/(float)(max_t - min_t);
    size_t size = m_vars.size();
    for (size_t i=0 ; i < size ; ++i)
    {
        m_vars[i] = (m_vars[i] - min_t)*inv_scale;
        m_vars[i] = m_vars[i]>0.0 ? m_vars[i] : 0.0;//max(m_vars[i], 0.0f);
        m_vars[i] = m_vars[i]<1.0 ? m_vars[i] : 1.0;//min(m_vars[i], 1.0f);
    }
}

template<typename T>
void HostArray<T>::getMinMax(T& min_t, T& max_t){
#ifdef WIN32
    std::pair< typename vector_type::iterator, typename vector_type::iterator >
        minmaxPair = std::minmax_element(m_vars.begin(), m_vars.end());
    min_t = *(minmaxPair.first);
    max_t = *(minmaxPair.second);
    // T range = max_t - min_t;
#else
    min_t = *( std::min_element(m_vars.begin(), m_vars.end()) );
    max_t = *( std::max_element(m_vars.begin(), m_vars.end()) );
#endif
    qDebug("min value=%f max value=%f",min_t, max_t);
}

template<typename T>
void HostArray<T>::logNormalize()
{
#ifdef WIN32
        std::pair< typename vector_type::iterator, typename vector_type::iterator >
                minmaxPair = std::minmax_element(m_vars.begin(), m_vars.end());
        T min_t = 0;//log(*(minmaxPair.first));
        T max_t = log(*(minmaxPair.second));
#else
        T min_t = 0;//*( std::min_element(m_vars.begin(), m_vars.end()) );
        T max_t = log( *( std::max_element(m_vars.begin(), m_vars.end()) ) );
#endif
        qDebug("log min value=%f log max value=%f",min_t, max_t);
        float inv_scale = 1.0f/(float)(max_t - min_t);
    int size = m_vars.size();
    for (int i=0 ; i < size ; ++i)
    {
        if (m_vars[i] < 1){
            m_vars[i] = 0;
        }else{
            m_vars[i] = (log(m_vars[i]) - min_t)*inv_scale;
        }
        m_vars[i] = max(m_vars[i], 0.0005f);
        m_vars[i] = min(m_vars[i], 0.9999f);
    }
}

template<typename T>
T& HostArray<T>::operator()( int n,... )
{//Given high dimensional index(eg. z,y,x), calculate its
    // equivalent global index in 1D array m_vars.
    //so that gIdx = z*ydim*xdim + y*xdim + x;
        //assert(n==m_ndim);
        Q_ASSERT_X(n==m_ndim, "HostArray<T>::operator()", "n != m_ndim");
    va_list ap;
    int i;
    va_start(ap, n);
    int idx = va_arg(ap, int);
    size_t gIdx = idx;
    for (i = 1; i < n; ++i ){
        idx = va_arg(ap, int);
        gIdx = (gIdx * m_dimsizes[i]) + idx;
    }
    va_end(ap);
    return m_vars[gIdx];
}

template<typename T>
T HostArray<T>::operator()( int n,... ) const
{//Given high dimensional index(eg. z,y,x), calculate its
    // equivalent global index in 1D array m_vars.
    //so that gIdx = z*ydim*xdim + y*xdim + x;
        //assert(n==m_ndim);
        Q_ASSERT_X(n==m_ndim, "HostArray<T>::operator()", "n != m_ndim");
    va_list ap;
    int i;
    va_start(ap, n);
    int idx = va_arg(ap, int);
    int gIdx= idx;
    for (i = 1; i < n; ++i ){
        idx = va_arg(ap, int);
        gIdx = (gIdx * m_dimsizes[i]) + idx;
    }
    va_end(ap);
    return m_vars[gIdx];
}

#ifdef USE_CUDA
template<typename T>
HostArray<T>& HostArray<T>::copy_dev2host(const DevArray<T>& d_array){
    void* dst = (void*)(&m_vars[0]);
    void* src = (void*)(d_array.m_vars);
    int dst_size = 1;
    for (int i = 0; i < m_ndim ; i++)
    {
        dst_size *= m_dimsizes[i];
    }

    int src_size = 1;
    for (int i = 0; i < d_array.m_ndim ; i++)
    {
        src_size *= d_array.m_dimsizes[i];
    }
    if (dst_size < src_size){
            printf("ostArray<T>::copy_dev2host() dst memory size %d is smaller than the src memory size %d. Use the smaller one.\n",dst_size, src_size);
    }
    int size = min(dst_size, src_size);
    printf("src=%x, size=%d",src,size);
    checkCudaErrors(cudaMemcpy(dst , src , size * sizeof(T), cudaMemcpyDeviceToHost));

    return *this;
}

template<typename T>
HostArray<T>& HostArray<T>::copy_dev2host( const T* d_pointer, size_t nElmnts )
{
    void* dst = (void*)(&m_vars[0]);
    int dst_size = 1;
    for (int i = 0; i < m_ndim ; i++)
    {
        dst_size *= m_dimsizes[i];
    }
    if (dst_size < nElmnts){
            printf("HostArray<T>::copy_dev2host() dst memory size %d is smaller than the src memory size %d. Use the smaller one.\n",dst_size, nElmnts);
    }
    size_t size = std::min((size_t)dst_size, nElmnts);
    checkCudaErrors(cudaMemcpy(dst , d_pointer , size * sizeof(T), cudaMemcpyDeviceToHost));

    return *this;
}
#endif

template<typename T>
void HostArray<T>::resize( size_t newSize )
{
    m_dimsizes[0]=newSize;
    int totalSize=newSize;
    for (int i = 1; i < m_ndim ; i++)
    {
        totalSize *= m_dimsizes[i];
    }
    m_vars.resize(totalSize);
}

template<typename T>
static inline ostream& output1dim(ostream& ofs, const HostArray<T> &t)
{
    int idx0=0;
    int dim0 = t.m_dimsizes[0];
    while (idx0 < dim0 - 1)
    {
        ofs << t.m_vars[idx0]<<endl;
        idx0++;
    }
    ofs << t.m_vars[idx0];
    return ofs;
}

template<typename T>
static inline ostream& output2dim(ostream& ofs, const HostArray<T> &t)
{
    int size = 1;
    for (int i = 0; i < t.m_ndim ; i++)
    {
        size *= t.m_dimsizes[i];
    }
    int idx0=0;
    int dim0 = t.m_dimsizes[0];
    int dim1 = t.m_dimsizes[1];
    while (idx0 < dim0 - 1)
    {
        for (int idx1=0; idx1 < dim1-1 ; idx1++)
        {
            int val = t.m_vars[idx0*dim1 + idx1];
            ofs << val <<" ";
        }
        ofs << t.m_vars[idx0*dim1 + dim1 - 1]<<endl;
        idx0++;
    }
    for (int idx1=0; idx1 < dim1-1 ; idx1++)
    {
        int val = t.m_vars[idx0*dim1 + idx1];
        ofs << val <<" ";
    }
    ofs << t.m_vars[idx0*dim1 + dim1 - 1];
    return ofs;
}

template<typename T>
ostream& operator << ( ostream& ofs, const HostArray<T> &t)
{
    switch (t.m_ndim)
    {
    case 1:
        return output1dim(ofs, t);

    case 2:
        return output2dim(ofs, t);
    }
    return ofs;
}
template<typename T>
istream& operator >> ( istream& is, const HostArray<T> &t)
{
    int i=0;
    while(!is.eof())
    {
        is >> (t.m_vars)[i++];
    }
    return is;
}
template<typename T>
ostream& operator << ( ostream& ofs, /*const*/ HostArray<T> &t)
{
    switch (t.m_ndim)
    {
    case 1:
        return output1dim(ofs, t);

    case 2:
        return output2dim(ofs, t);
    }
    return ofs;
}
template<typename T>
istream& operator >> ( istream& is, /*const*/ HostArray<T> &t)
{
    int i=0;
    while(!is.eof())
    {
        is >> (t.m_vars)[i++];
    }
    return is;
}
typedef unsigned char uchar;
typedef HostArray<uchar> HostUCharArrayType;
typedef HostArray<uchar> HostByteArrayType;
typedef HostArray<int> 	 HostIntArrayType;
typedef HostArray<unsigned int> HostUIntArrayType;
typedef HostArray<float>  HostFloatArrayType;
typedef HostArray<double> HostDoubleArrayType;

typedef std::shared_ptr<HostUCharArrayType>  HostUCharArrayRef;
typedef std::shared_ptr<HostUCharArrayType>  HostByteArrayRef;
typedef std::shared_ptr<HostIntArrayType>    HostIntArrayRef;
typedef std::shared_ptr<HostUIntArrayType>   HostUIntArrayRef;
typedef std::shared_ptr<HostFloatArrayType>  HostFloatArrayRef;
typedef std::shared_ptr<HostDoubleArrayType> HostDoubleArrayRef;

#endif
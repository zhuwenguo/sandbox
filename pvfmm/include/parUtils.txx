#include <cmath>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <ompUtils.h>
#include <matrix.hpp>

namespace pvfmm{
namespace par{

  template<typename T>
  int HyperQuickSort(const Vector<T>& arr_, Vector<T>& SortedElem){
    srand(0);
    long long nelem = arr_.Dim();
    Vector<T> arr=arr_;
    merge_sort(&arr[0], &arr[0]+nelem);
    SortedElem.Resize(nelem);
    memcpy(&SortedElem[0], &arr[0], nelem*sizeof(T));
    return 0;
  }


  template<typename T>
  int SortScatterIndex(const Vector<T>& key, Vector<size_t>& scatter_index, const T* split_key_){
    typedef SortPair<T,size_t> Pair_t;
    Vector<Pair_t> parray(key.Dim());
    long long loc_size=key.Dim();
#pragma omp parallel for
    for(size_t i=0;i<loc_size;i++){
      parray[i].key=key[i];
      parray[i].data=i;
    }
    Vector<Pair_t> psorted;
    HyperQuickSort(parray, psorted);
    scatter_index.Resize(psorted.Dim());
#pragma omp parallel for
    for(size_t i=0;i<psorted.Dim();i++){
      scatter_index[i]=psorted[i].data;
    }
    return 0;
  }

  template<typename T>
  int ScatterForward(Vector<T>& data_, const Vector<size_t>& scatter_index){
    typedef SortPair<size_t,size_t> Pair_t;
    size_t data_dim=0;
    long long send_size=0;
    long long recv_size=0;
    {
      recv_size=scatter_index.Dim();
      long long loc_size[2]={(long long)(data_.Dim()*sizeof(T)), recv_size};
      if(loc_size[0]==0 || loc_size[1]==0) return 0;
      data_dim=loc_size[0]/loc_size[1];
      send_size=(data_.Dim()*sizeof(T))/data_dim;
    }
    Vector<char> recv_buff(recv_size*data_dim);
    Vector<Pair_t> psorted(recv_size);
    {
#pragma omp parallel for
      for(size_t i=0;i<recv_size;i++){
	psorted[i].key=scatter_index[i];
	psorted[i].data=i;
      }
      merge_sort(&psorted[0], &psorted[0]+recv_size);
    }
    {
      char* data=(char*)&data_[0];
#pragma omp parallel for
      for(size_t i=0;i<send_size;i++){
	size_t src_indx=psorted[i].key*data_dim;
	size_t trg_indx=i*data_dim;
	for(size_t j=0;j<data_dim;j++) {
	  recv_buff[trg_indx+j]=data[src_indx+j];
	}
      }
    }

    {
      char* data=(char*)&data_[0];
#pragma omp parallel for
      for(size_t i=0;i<recv_size;i++){
	size_t src_indx=i*data_dim;
	size_t trg_indx=psorted[i].data*data_dim;
	for(size_t j=0;j<data_dim;j++)
	  data[trg_indx+j]=recv_buff[src_indx+j];
      }
    }
    return 0;
  }

}//end namespace
}//end namespace

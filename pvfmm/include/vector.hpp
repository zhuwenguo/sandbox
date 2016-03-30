#ifndef _PVFMM_VECTOR_HPP_
#define _PVFMM_VECTOR_HPP_

namespace pvfmm{

template <class T>
class Vector{

public:

  struct
  Device{
    Device& operator=(Vector& V){
      dim=V.Dim();
      dev_ptr=(uintptr_t)&V[0];
      return *this;
    }
    inline T& operator[](size_t j) const{
      return ((T*)dev_ptr)[j];
    }
    size_t dim;
    uintptr_t dev_ptr;
  };

  Vector(){
    dim=0;
    capacity=0;
    own_data=true;
    data_ptr=NULL;
    dev.dev_ptr=(uintptr_t)NULL;
  }

  Vector(size_t dim_, T* data_=NULL, bool own_data_=true){
    dim=dim_;
    capacity=dim;
    own_data=own_data_;
    if(own_data){
      if(dim>0){
	data_ptr=mem::aligned_new<T>(capacity);
	if(data_!=NULL) memcpy(data_ptr,data_,dim*sizeof(T));
      }else data_ptr=NULL;
    }else
      data_ptr=data_;
    dev.dev_ptr=(uintptr_t)NULL;
  }

  Vector(const Vector<T>& V){
    dim=V.dim;
    capacity=dim;
    own_data=true;
    if(dim>0){
      data_ptr=mem::aligned_new<T>(capacity);
      memcpy(data_ptr,V.data_ptr,dim*sizeof(T));
    }else
      data_ptr=NULL;
    dev.dev_ptr=(uintptr_t)NULL;
  }

  Vector(const std::vector<T>& V){
    dim=V.size();
    capacity=dim;
    own_data=true;
    if(dim>0){
      data_ptr=mem::aligned_new<T>(capacity);
      memcpy(data_ptr,&V[0],dim*sizeof(T));
    }else
      data_ptr=NULL;
    dev.dev_ptr=(uintptr_t)NULL;
  }

  ~Vector(){
    FreeDevice(false);
    if(own_data){
      if(data_ptr!=NULL){
	mem::aligned_delete(data_ptr);
      }
    }
    data_ptr=NULL;
    capacity=0;
    dim=0;
  }

  void Swap(Vector<T>& v1){
    size_t dim_=dim;
    size_t capacity_=capacity;
    T* data_ptr_=data_ptr;
    bool own_data_=own_data;
    Device dev_=dev;

    dim=v1.dim;
    capacity=v1.capacity;
    data_ptr=v1.data_ptr;
    own_data=v1.own_data;
    dev=v1.dev;

    v1.dim=dim_;
    v1.capacity=capacity_;
    v1.data_ptr=data_ptr_;
    v1.own_data=own_data_;
    v1.dev=dev_;
  }

  void ReInit(size_t dim_, T* data_=NULL, bool own_data_=true){
    if(own_data_ && own_data && dim_<=capacity){
      if(dim!=dim_) FreeDevice(false); dim=dim_;
      if(data_) memcpy(data_ptr,data_,dim*sizeof(T));
    }else{
      Vector<T> tmp(dim_,data_,own_data_);
      this->Swap(tmp);
    }
  }

  void FreeDevice(bool copy){
    if(dev.dev_ptr==(uintptr_t)NULL) return;
    dev.dev_ptr=(uintptr_t)NULL;
    dev.dim=0;
  }

  void Write(const char* fname){
    FILE* f1=fopen(fname,"wb+");
    if(f1==NULL){
      std::cout<<"Unable to open file for writing:"<<fname<<'\n';
      return;
    }
    int dim_=dim;
    fwrite(&dim_,sizeof(int),2,f1);
    fwrite(data_ptr,sizeof(T),dim,f1);
    fclose(f1);
  }

  inline size_t Dim() const{
    return dim;
  }

  inline size_t Capacity() const{
    return capacity;
  }

  void Resize(size_t dim_){
    if(dim!=dim_) FreeDevice(false);
    if(capacity>=dim_) dim=dim_;
    else ReInit(dim_);
  }

  void SetZero(){
    if(dim>0)
      memset(data_ptr,0,dim*sizeof(T));
  }

  Vector<T>& operator=(const Vector<T>& V){
    if(this!=&V){
      if(dim!=V.dim) FreeDevice(false);
      if(capacity<V.dim) ReInit(V.dim); dim=V.dim;
      memcpy(data_ptr,V.data_ptr,dim*sizeof(T));
    }
    return *this;
  }

  Vector<T>& operator=(const std::vector<T>& V){
    {
      if(dim!=V.size()) FreeDevice(false);
      if(capacity<V.size()) ReInit(V.size()); dim=V.size();
      memcpy(data_ptr,&V[0],dim*sizeof(T));
    }
    return *this;
  }

  inline T& operator[](size_t j) const{
    assert(dim>0?j<dim:j==0); //TODO Change to (j<dim)
    return data_ptr[j];
  }

private:
  size_t dim;
  size_t capacity;
  T* data_ptr;
  bool own_data;
  Device dev;
};

}//end namespace

#endif //_PVFMM_VECTOR_HPP_

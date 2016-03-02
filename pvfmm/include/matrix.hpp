#ifndef _PVFMM_MATRIX_HPP_
#define _PVFMM_MATRIX_HPP_

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <omp.h>
#include <stdint.h>
#include <vector>

#include <mem_mgr.hpp>
#include <profile.hpp>
#include <pvfmm_common.hpp>
#include <vector.hpp>

extern "C" {
  void sgemm_(char* TRANSA, char* TRANSB, int* M, int* N, int* K, float* ALPHA, float* A,
	      int* LDA, float* B, int* LDB, float* BETA, float* C, int* LDC);
  void dgemm_(char* TRANSA, char* TRANSB, int* M, int* N, int* K, double* ALPHA, double* A,
	      int* LDA, double* B, int* LDB, double* BETA, double* C, int* LDC);
  void sgesvd_(char *JOBU, char *JOBVT, int *M, int *N, float *A, int *LDA,
	       float *S, float *U, int *LDU, float *VT, int *LDVT, float *WORK, int *LWORK, int *INFO);
  void dgesvd_(char *JOBU, char *JOBVT, int *M, int *N, double *A, int *LDA,
	       double *S, double *U, int *LDU, double *VT, int *LDVT, double *WORK, int *LWORK, int *INFO);
}

namespace pvfmm{

  template <class T>
  void tgemm(char TransA, char TransB,  int M,  int N,  int K,  T alpha,  T *A,  int lda,  T *B,  int ldb,  T beta, T *C,  int ldc) {
    sgemm_(&TransA, &TransB, &M, &N, &K, &alpha, A, &lda, B, &ldb, &beta, C, &ldc);
  }

  template<>
  void tgemm<double>(char TransA, char TransB,  int M,  int N,  int K,  double alpha,  double *A,  int lda,  double *B,  int ldb,  double beta, double *C,  int ldc){
    dgemm_(&TransA, &TransB, &M, &N, &K, &alpha, A, &lda, B, &ldb, &beta, C, &ldc);
  }

  template <class T>
  void tsvd(char *JOBU, char *JOBVT, int *M, int *N, T *A, int *LDA,
	    T *S, T *U, int *LDU, T *VT, int *LDVT, T *WORK, int *LWORK,
	    int *INFO) {
    sgesvd_(JOBU,JOBVT,M,N,A,LDA,S,U,LDU,VT,LDVT,WORK,LWORK,INFO);
  }

  template<>
  void tsvd<double>(char *JOBU, char *JOBVT, int *M, int *N, double *A, int *LDA,
		    double *S, double *U, int *LDU, double *VT, int *LDVT, double *WORK, int *LWORK, int *INFO){
    dgesvd_(JOBU,JOBVT,M,N,A,LDA,S,U,LDU,VT,LDVT,WORK,LWORK,INFO);
  }

  template <class T>
  void tpinv(T* M, int n1, int n2, T eps, T* M_){
    if(n1*n2==0) return;
    int m = n2;
    int n = n1;
    int k = (m<n?m:n);
    T* tU =mem::aligned_new<T>(m*k);
    T* tS =mem::aligned_new<T>(k);
    T* tVT=mem::aligned_new<T>(k*n);
    int INFO=0;
    char JOBU  = 'S';
    char JOBVT = 'S';
    int wssize = 3*(m<n?m:n)+(m>n?m:n);
    int wssize1 = 5*(m<n?m:n);
    wssize = (wssize>wssize1?wssize:wssize1);
    T* wsbuf = mem::aligned_new<T>(wssize);
    tsvd(&JOBU, &JOBVT, &m, &n, &M[0], &m, &tS[0], &tU[0], &m, &tVT[0], &k,
        wsbuf, &wssize, &INFO);
    if(INFO!=0)
      std::cout<<INFO<<'\n';
    assert(INFO==0);
    mem::aligned_delete<T>(wsbuf);
    T eps_=tS[0]*eps;
    for(int i=0;i<k;i++)
      if(tS[i]<eps_)
        tS[i]=0;
      else
        tS[i]=1.0/tS[i];
    for(int i=0;i<m;i++){
      for(int j=0;j<k;j++){
        tU[i+j*m]*=tS[j];
      }
    }
    tgemm<T>('T','T',n,m,k,1.0,&tVT[0],k,&tU[0],m,0.0,M_,n);
    mem::aligned_delete<T>(tU);
    mem::aligned_delete<T>(tS);
    mem::aligned_delete<T>(tVT);
  }

template <class T>
class Permutation;

template <class T>
class Matrix{

public:
  struct Device{
    Device& operator=(Matrix& M){
      dim[0]=M.Dim(0);
      dim[1]=M.Dim(1);
      dev_ptr=(uintptr_t)M.data_ptr;
      return *this;
    }
    inline T* operator[](size_t j) const{
      assert(j<dim[0]);
      return &((T*)dev_ptr)[j*dim[1]];
    }
    size_t dim[2];
    uintptr_t dev_ptr;
    int lock_idx;
  };

  private:

  size_t dim[2];
  T* data_ptr;
  bool own_data;

  Device dev;
  Vector<char> dev_sig;

  static inline void gemm(char TransA, char TransB,  int M,  int N,  int K,  T alpha,  T *A,  int lda,  T *B,  int ldb,  T beta, T *C,  int ldc){
    if((TransA=='N' || TransA=='n') && (TransB=='N' || TransB=='n')){
      for(size_t n=0;n<N;n++){
        for(size_t m=0;m<M;m++){
            T AxB=0;
            for(size_t k=0;k<K;k++){
              AxB+=A[m+lda*k]*B[k+ldb*n];
            }
            C[m+ldc*n]=alpha*AxB+(beta==0?0:beta*C[m+ldc*n]);
        }
      }
    }else if(TransA=='N' || TransA=='n'){
      for(size_t n=0;n<N;n++){
        for(size_t m=0;m<M;m++){
            T AxB=0;
            for(size_t k=0;k<K;k++){
              AxB+=A[m+lda*k]*B[n+ldb*k];
            }
            C[m+ldc*n]=alpha*AxB+(beta==0?0:beta*C[m+ldc*n]);
        }
      }
    }else if(TransB=='N' || TransB=='n'){
      for(size_t n=0;n<N;n++){
        for(size_t m=0;m<M;m++){
            T AxB=0;
            for(size_t k=0;k<K;k++){
              AxB+=A[k+lda*m]*B[k+ldb*n];
            }
            C[m+ldc*n]=alpha*AxB+(beta==0?0:beta*C[m+ldc*n]);
        }
      }
    }else{
      for(size_t n=0;n<N;n++){
        for(size_t m=0;m<M;m++){
            T AxB=0;
            for(size_t k=0;k<K;k++){
              AxB+=A[k+lda*m]*B[n+ldb*k];
            }
            C[m+ldc*n]=alpha*AxB+(beta==0?0:beta*C[m+ldc*n]);
        }
      }
    }
  }

  public:


  Matrix(){
    dim[0]=0;
    dim[1]=0;
    own_data=true;
    data_ptr=NULL;
    dev.dev_ptr=(uintptr_t)NULL;
  }

  Matrix(size_t dim1, size_t dim2, T* data_=NULL, bool own_data_=true) {
    dim[0]=dim1;
    dim[1]=dim2;
    own_data=own_data_;
    if(own_data){
      if(dim[0]*dim[1]>0){
	data_ptr=mem::aligned_new<T>(dim[0]*dim[1]);
	if(data_!=NULL) mem::memcopy(data_ptr,data_,dim[0]*dim[1]*sizeof(T));
      }else data_ptr=NULL;
    }else
      data_ptr=data_;
    dev.dev_ptr=(uintptr_t)NULL;
  }

  Matrix(const Matrix<T>& M){
    dim[0]=M.dim[0];
    dim[1]=M.dim[1];

    own_data=true;
    if(dim[0]*dim[1]>0){
      data_ptr=mem::aligned_new<T>(dim[0]*dim[1]);
      mem::memcopy(data_ptr,M.data_ptr,dim[0]*dim[1]*sizeof(T));
    }else
      data_ptr=NULL;
    dev.dev_ptr=(uintptr_t)NULL;
  }

  ~Matrix(){
    FreeDevice(false);
    if(own_data){
      if(data_ptr!=NULL){
	mem::aligned_delete(data_ptr);
      }
    }
    data_ptr=NULL;
    dim[0]=0;
    dim[1]=0;
  }

  void Swap(Matrix<T>& M){
    size_t dim_[2]={dim[0],dim[1]};
    T* data_ptr_=data_ptr;
    bool own_data_=own_data;
    Device dev_=dev;
    Vector<char> dev_sig_=dev_sig;

    dim[0]=M.dim[0];
    dim[1]=M.dim[1];
    data_ptr=M.data_ptr;
    own_data=M.own_data;
    dev=M.dev;
    dev_sig=M.dev_sig;

    M.dim[0]=dim_[0];
    M.dim[1]=dim_[1];
    M.data_ptr=data_ptr_;
    M.own_data=own_data_;
    M.dev=dev_;
    M.dev_sig=dev_sig_;
  }

  void ReInit(size_t dim1, size_t dim2, T* data_=NULL, bool own_data_=true){
    if(own_data_ && own_data && dim[0]*dim[1]>=dim1*dim2){
      if(dim[0]*dim[1]!=dim1*dim2) FreeDevice(false);
      dim[0]=dim1; dim[1]=dim2;
      if(data_) mem::memcopy(data_ptr,data_,dim[0]*dim[1]*sizeof(T));
    }else{
      Matrix<T> tmp(dim1,dim2,data_,own_data_);
      this->Swap(tmp);
    }
  }

  void FreeDevice(bool copy){
    if(dev.dev_ptr==(uintptr_t)NULL) return;
    dev.dev_ptr=(uintptr_t)NULL;
    dev.dim[0]=0;
    dev.dim[1]=0;
  }

  void Write(const char* fname){
    FILE* f1=fopen(fname,"wb+");
    if(f1==NULL){
      std::cout<<"Unable to open file for writing:"<<fname<<'\n';
      return;
    }
    uint32_t dim_[2]={(uint32_t)dim[0],(uint32_t)dim[1]};
    fwrite(dim_,sizeof(uint32_t),2,f1);
    fwrite(data_ptr,sizeof(T),dim[0]*dim[1],f1);
    fclose(f1);
  }

  void Read(const char* fname){
    FILE* f1=fopen(fname,"r");
    if(f1==NULL){
      std::cout<<"Unable to open file for reading:"<<fname<<'\n';
      return;
    }
    uint32_t dim_[2];
    size_t readlen=fread (dim_, sizeof(uint32_t), 2, f1);
    assert(readlen==2);

    ReInit(dim_[0],dim_[1]);
    readlen=fread(data_ptr,sizeof(T),dim[0]*dim[1],f1);
    assert(readlen==dim[0]*dim[1]);
    fclose(f1);
  }

  size_t Dim(size_t i) const{
    return dim[i];
  }

  void Resize(size_t i, size_t j){
    if(dim[0]*dim[1]!=i*j) FreeDevice(false);
    if(dim[0]*dim[1]>=i*j){
      dim[0]=i; dim[1]=j;
    }else ReInit(i,j);
  }

  void SetZero(){
    if(dim[0]*dim[1])
      memset(data_ptr,0,dim[0]*dim[1]*sizeof(T));
  }

  Matrix<T>& operator=(const Matrix<T>& M){
    if(this!=&M){
      if(dim[0]*dim[1]!=M.dim[0]*M.dim[1]) FreeDevice(false);
      if(dim[0]*dim[1]<M.dim[0]*M.dim[1]){
	ReInit(M.dim[0],M.dim[1]);
      }
      dim[0]=M.dim[0]; dim[1]=M.dim[1];
      mem::memcopy(data_ptr,M.data_ptr,dim[0]*dim[1]*sizeof(T));
    }
    return *this;
  }

  Matrix<T>& operator+=(const Matrix<T>& M){
    assert(M.Dim(0)==Dim(0) && M.Dim(1)==Dim(1));
    Profile::Add_FLOP(dim[0]*dim[1]);

    for(size_t i=0;i<M.Dim(0)*M.Dim(1);i++)
      data_ptr[i]+=M.data_ptr[i];
    return *this;
  }

  Matrix<T>& operator-=(const Matrix<T>& M){
    assert(M.Dim(0)==Dim(0) && M.Dim(1)==Dim(1));
    Profile::Add_FLOP(dim[0]*dim[1]);

    for(size_t i=0;i<M.Dim(0)*M.Dim(1);i++)
      data_ptr[i]-=M.data_ptr[i];
    return *this;
  }

  Matrix<T> operator+(const Matrix<T>& M2){
    Matrix<T>& M1=*this;
    assert(M2.Dim(0)==M1.Dim(0) && M2.Dim(1)==M1.Dim(1));
    Profile::Add_FLOP(dim[0]*dim[1]);
    Matrix<T> M_r(M1.Dim(0),M1.Dim(1),NULL);
    for(size_t i=0;i<M1.Dim(0)*M1.Dim(1);i++)
      M_r[0][i]=M1[0][i]+M2[0][i];
    return M_r;
  }

  Matrix<T> operator-(const Matrix<T>& M2){
    Matrix<T>& M1=*this;
    assert(M2.Dim(0)==M1.Dim(0) && M2.Dim(1)==M1.Dim(1));
    Profile::Add_FLOP(dim[0]*dim[1]);
    Matrix<T> M_r(M1.Dim(0),M1.Dim(1),NULL);
    for(size_t i=0;i<M1.Dim(0)*M1.Dim(1);i++)
      M_r[0][i]=M1[0][i]-M2[0][i];
    return M_r;
  }

  inline T& operator()(size_t i,size_t j) const{
    assert(i<dim[0] && j<dim[1]);
    return data_ptr[i*dim[1]+j];
  }

  inline T* operator[](size_t i) const{
    assert(i<dim[0]);
    return &data_ptr[i*dim[1]];
  }

  Matrix<T> operator*(const Matrix<T>& M){
    assert(dim[1]==M.dim[0]);
    Profile::Add_FLOP(2*(((long long)dim[0])*dim[1])*M.dim[1]);
    Matrix<T> M_r(dim[0],M.dim[1],NULL);
    if(M.Dim(0)*M.Dim(1)==0 || this->Dim(0)*this->Dim(1)==0) return M_r;
    gemm('N','N',M.dim[1],dim[0],dim[1],
		 1.0,M.data_ptr,M.dim[1],data_ptr,dim[1],0.0,M_r.data_ptr,M_r.dim[1]);
    return M_r;
  }

  static void GEMM(Matrix<T>& M_r, const Matrix<T>& A, const Matrix<T>& B, T beta=0.0){
    if(A.Dim(0)*A.Dim(1)==0 || B.Dim(0)*B.Dim(1)==0) return;
    assert(A.dim[1]==B.dim[0]);
    assert(M_r.dim[0]==A.dim[0]);
    assert(M_r.dim[1]==B.dim[1]);
    gemm('N','N',B.dim[1],A.dim[0],A.dim[1],
		 1.0,B.data_ptr,B.dim[1],A.data_ptr,A.dim[1],beta,M_r.data_ptr,M_r.dim[1]);
  }

#define myswap(t,a,b) {t c=a;a=b;b=c;}

  void RowPerm(const Permutation<T>& P){
    Matrix<T>& M=*this;
    if(P.Dim()==0) return;
    assert(M.Dim(0)==P.Dim());
    size_t d0=M.Dim(0);
    size_t d1=M.Dim(1);

#pragma omp parallel for
    for(size_t i=0;i<d0;i++){
    T* M_=M[i];
    const T s=P.scal[i];
    for(size_t j=0;j<d1;j++) M_[j]*=s;
  }

    Permutation<T> P_=P;
    for(size_t i=0;i<d0;i++)
      while(P_.perm[i]!=i){
    size_t a=P_.perm[i];
    size_t b=i;
    T* M_a=M[a];
    T* M_b=M[b];
    myswap(size_t,P_.perm[a],P_.perm[b]);
    for(size_t j=0;j<d1;j++)
      myswap(T,M_a[j],M_b[j]);
  }
  }

  void ColPerm(const Permutation<T>& P){
    Matrix<T>& M=*this;
    if(P.Dim()==0) return;
    assert(M.Dim(1)==P.Dim());
    size_t d0=M.Dim(0);
    size_t d1=M.Dim(1);

    int omp_p=omp_get_max_threads();
    Matrix<T> M_buff(omp_p,d1);

    const size_t* perm_=&(P.perm[0]);
    const T* scal_=&(P.scal[0]);
#pragma omp parallel for
    for(size_t i=0;i<d0;i++){
      int pid=omp_get_thread_num();
      T* buff=&M_buff[pid][0];
      T* M_=M[i];
      for(size_t j=0;j<d1;j++)
	buff[j]=M_[j];
      for(size_t j=0;j<d1;j++){
	M_[j]=buff[perm_[j]]*scal_[j];
      }
    }
  }
#undef myswap

#define B1 128
#define B2 32

  Matrix<T> Transpose(){
    Matrix<T>& M=*this;
    size_t d0=M.dim[0];
    size_t d1=M.dim[1];
    Matrix<T> M_r(d1,d0,NULL);

    const size_t blk0=((d0+B1-1)/B1);
    const size_t blk1=((d1+B1-1)/B1);
    const size_t blks=blk0*blk1;
    for(size_t k=0;k<blks;k++){
      size_t i=(k%blk0)*B1;
      size_t j=(k/blk0)*B1;
      size_t d0_=i+B1; if(d0_>=d0) d0_=d0;
      size_t d1_=j+B1; if(d1_>=d1) d1_=d1;
      for(size_t ii=i;ii<d0_;ii+=B2)
	for(size_t jj=j;jj<d1_;jj+=B2){
	  size_t d0__=ii+B2; if(d0__>=d0) d0__=d0;
	  size_t d1__=jj+B2; if(d1__>=d1) d1__=d1;
	  for(size_t iii=ii;iii<d0__;iii++)
	    for(size_t jjj=jj;jjj<d1__;jjj++){
	      M_r[jjj][iii]=M[iii][jjj];
	    }
	}
    }
    return M_r;
  }

  void Transpose(Matrix<T>& M_r, const Matrix<T>& M){
    size_t d0=M.dim[0];
    size_t d1=M.dim[1];
    M_r.Resize(d1, d0);

    const size_t blk0=((d0+B1-1)/B1);
    const size_t blk1=((d1+B1-1)/B1);
    const size_t blks=blk0*blk1;
#pragma omp parallel for
    for(size_t k=0;k<blks;k++){
      size_t i=(k%blk0)*B1;
      size_t j=(k/blk0)*B1;
      size_t d0_=i+B1; if(d0_>=d0) d0_=d0;
      size_t d1_=j+B1; if(d1_>=d1) d1_=d1;
      for(size_t ii=i;ii<d0_;ii+=B2)
	for(size_t jj=j;jj<d1_;jj+=B2){
	  size_t d0__=ii+B2; if(d0__>=d0) d0__=d0;
	  size_t d1__=jj+B2; if(d1__>=d1) d1__=d1;
	  for(size_t iii=ii;iii<d0__;iii++)
	    for(size_t jjj=jj;jjj<d1__;jjj++){
	      M_r[jjj][iii]=M[iii][jjj];
	    }
	}
    }
  }
#undef B2
#undef B1

  #define U(i,j) U_[(i)*dim[0]+(j)]
  #define S(i,j) S_[(i)*dim[1]+(j)]
  #define V(i,j) V_[(i)*dim[1]+(j)]

  static void GivensL(T* S_, const size_t dim[2], size_t m, T a, T b){
    T r=sqrtf(a*a+b*b);
    T c=a/r;
    T s=-b/r;
#pragma omp parallel for
    for(size_t i=0;i<dim[1];i++){
      T S0=S(m+0,i);
      T S1=S(m+1,i);
      S(m  ,i)+=S0*(c-1);
      S(m  ,i)+=S1*(-s );
      S(m+1,i)+=S0*( s );
      S(m+1,i)+=S1*(c-1);
    }
  }

  static void GivensR(T* S_, const size_t dim[2], size_t m, T a, T b){
    T r=sqrtf(a*a+b*b);
    T c=a/r;
  T s=-b/r;
#pragma omp parallel for
    for(size_t i=0;i<dim[0];i++){
      T S0=S(i,m+0);
      T S1=S(i,m+1);
      S(i,m  )+=S0*(c-1);
      S(i,m  )+=S1*(-s );
      S(i,m+1)+=S0*( s );
      S(i,m+1)+=S1*(c-1);
    }
  }

  static void SVD5(const size_t dim[2], T* U_, T* S_, T* V_, T eps=-1){
    assert(dim[0]>=dim[1]);
    {
      size_t n=std::min(dim[0],dim[1]);
      std::vector<T> house_vec(std::max(dim[0],dim[1]));
      for(size_t i=0;i<n;i++){
        {
          T x1=S(i,i);
          if(x1<0) x1=-x1;
          T x_inv_norm=0;
          for(size_t j=i;j<dim[0];j++){
            x_inv_norm+=S(j,i)*S(j,i);
          }
          if(x_inv_norm>0) x_inv_norm=1/sqrtf(x_inv_norm);
          T alpha=sqrtf(1+x1*x_inv_norm);
          T beta=x_inv_norm/alpha;
          house_vec[i]=-alpha;
          for(size_t j=i+1;j<dim[0];j++){
            house_vec[j]=-beta*S(j,i);
          }
          if(S(i,i)<0) for(size_t j=i+1;j<dim[0];j++){
            house_vec[j]=-house_vec[j];
          }
        }
#pragma omp parallel for
        for(size_t k=i;k<dim[1];k++){
          T dot_prod=0;
          for(size_t j=i;j<dim[0];j++){
            dot_prod+=S(j,k)*house_vec[j];
          }
          for(size_t j=i;j<dim[0];j++){
            S(j,k)-=dot_prod*house_vec[j];
          }
        }
#pragma omp parallel for
        for(size_t k=0;k<dim[0];k++){
          T dot_prod=0;
          for(size_t j=i;j<dim[0];j++){
            dot_prod+=U(k,j)*house_vec[j];
          }
          for(size_t j=i;j<dim[0];j++){
            U(k,j)-=dot_prod*house_vec[j];
          }
        }
        if(i>=n-1) continue;
        {
          T x1=S(i,i+1);
          if(x1<0) x1=-x1;
          T x_inv_norm=0;
          for(size_t j=i+1;j<dim[1];j++){
            x_inv_norm+=S(i,j)*S(i,j);
          }
          if(x_inv_norm>0) x_inv_norm=1/sqrtf(x_inv_norm);
          T alpha=sqrtf(1+x1*x_inv_norm);
          T beta=x_inv_norm/alpha;
          house_vec[i+1]=-alpha;
          for(size_t j=i+2;j<dim[1];j++){
            house_vec[j]=-beta*S(i,j);
          }
          if(S(i,i+1)<0) for(size_t j=i+2;j<dim[1];j++){
            house_vec[j]=-house_vec[j];
          }
        }
#pragma omp parallel for
        for(size_t k=i;k<dim[0];k++){
          T dot_prod=0;
          for(size_t j=i+1;j<dim[1];j++){
            dot_prod+=S(k,j)*house_vec[j];
          }
          for(size_t j=i+1;j<dim[1];j++){
            S(k,j)-=dot_prod*house_vec[j];
          }
        }
#pragma omp parallel for
        for(size_t k=0;k<dim[1];k++){
          T dot_prod=0;
          for(size_t j=i+1;j<dim[1];j++){
            dot_prod+=V(j,k)*house_vec[j];
          }
          for(size_t j=i+1;j<dim[1];j++){
            V(j,k)-=dot_prod*house_vec[j];
          }
        }
      }
    }
    size_t k0=0;
    size_t iter=0;
    if(eps<0){
      eps=1.0;
      while(eps+(T)1.0>1.0) eps*=0.5;
      eps*=64.0;
    }
    while(k0<dim[1]-1){
      iter++;
      T S_max=0.0;
      for(size_t i=0;i<dim[1];i++) S_max=(S_max>S(i,i)?S_max:S(i,i));
      while(k0<dim[1]-1 && fabs(S(k0,k0+1))<=eps*S_max) k0++;
      if(k0==dim[1]-1) continue;
      size_t n=k0+2;
      while(n<dim[1] && fabs(S(n-1,n))>eps*S_max) n++;
      T alpha=0;
      T beta=0;
      {
        T C[2][2];
        C[0][0]=S(n-2,n-2)*S(n-2,n-2);
        if(n-k0>2) C[0][0]+=S(n-3,n-2)*S(n-3,n-2);
        C[0][1]=S(n-2,n-2)*S(n-2,n-1);
        C[1][0]=S(n-2,n-2)*S(n-2,n-1);
        C[1][1]=S(n-1,n-1)*S(n-1,n-1)+S(n-2,n-1)*S(n-2,n-1);
        T b=-(C[0][0]+C[1][1])/2;
        T c=  C[0][0]*C[1][1] - C[0][1]*C[1][0];
        T d=0;
        if(b*b-c>0) d=sqrtf(b*b-c);
        else{
          T b=(C[0][0]-C[1][1])/2;
          T c=-C[0][1]*C[1][0];
          if(b*b-c>0) d=sqrtf(b*b-c);
        }
        T lambda1=-b+d;
        T lambda2=-b-d;
        T d1=lambda1-C[1][1]; d1=(d1<0?-d1:d1);
        T d2=lambda2-C[1][1]; d2=(d2<0?-d2:d2);
        T mu=(d1<d2?lambda1:lambda2);
        alpha=S(k0,k0)*S(k0,k0)-mu;
        beta=S(k0,k0)*S(k0,k0+1);
      }
      for(size_t k=k0;k<n-1;k++){
        size_t dimU[2]={dim[0],dim[0]};
        size_t dimV[2]={dim[1],dim[1]};
        GivensR(S_,dim ,k,alpha,beta);
        GivensL(V_,dimV,k,alpha,beta);
        alpha=S(k,k);
        beta=S(k+1,k);
        GivensL(S_,dim ,k,alpha,beta);
        GivensR(U_,dimU,k,alpha,beta);
        alpha=S(k,k+1);
        beta=S(k,k+2);
      }
      {
        for(size_t i0=k0;i0<n-1;i0++){
          for(size_t i1=0;i1<dim[1];i1++){
            if(i0>i1 || i0+1<i1) S(i0,i1)=0;
          }
        }
        for(size_t i0=0;i0<dim[0];i0++){
          for(size_t i1=k0;i1<n-1;i1++){
            if(i0>i1 || i0+1<i1) S(i0,i1)=0;
          }
        }
        for(size_t i=0;i<dim[1]-1;i++){
          if(fabs(S(i,i+1))<=eps*S_max){
            S(i,i+1)=0;
          }
        }
      }
    }
  }

  #undef U
  #undef S
  #undef V

  static inline void svd(char *JOBU, char *JOBVT, int *M, int *N, T *A, int *LDA,
      T *S, T *U, int *LDU, T *VT, int *LDVT, T *WORK, int *LWORK,
      int *INFO){
    const size_t dim[2]={(size_t)std::max(*N,*M), (size_t)std::min(*N,*M)};
    T* U_=mem::aligned_new<T>(dim[0]*dim[0]); memset(U_, 0, dim[0]*dim[0]*sizeof(T));
    T* V_=mem::aligned_new<T>(dim[1]*dim[1]); memset(V_, 0, dim[1]*dim[1]*sizeof(T));
    T* S_=mem::aligned_new<T>(dim[0]*dim[1]);
    const size_t lda=*LDA;
    const size_t ldu=*LDU;
    const size_t ldv=*LDVT;
    if(dim[1]==*M){
      for(size_t i=0;i<dim[0];i++)
      for(size_t j=0;j<dim[1];j++){
        S_[i*dim[1]+j]=A[i*lda+j];
      }
    }else{
      for(size_t i=0;i<dim[0];i++)
      for(size_t j=0;j<dim[1];j++){
        S_[i*dim[1]+j]=A[j*lda+i];
      }
    }
    for(size_t i=0;i<dim[0];i++){
      U_[i*dim[0]+i]=1;
    }
    for(size_t i=0;i<dim[1];i++){
      V_[i*dim[1]+i]=1;
    }
    SVD5(dim, U_, S_, V_, (T)-1);
    for(size_t i=0;i<dim[1];i++){
      S[i]=S_[i*dim[1]+i];
    }
    if(dim[1]==*M){
      for(size_t i=0;i<dim[1];i++)
      for(size_t j=0;j<*M;j++){
        U[j+ldu*i]=V_[j+i*dim[1]]*(S[i]<0.0?-1.0:1.0);
      }
    }else{
      for(size_t i=0;i<dim[1];i++)
      for(size_t j=0;j<*M;j++){
        U[j+ldu*i]=U_[i+j*dim[0]]*(S[i]<0.0?-1.0:1.0);
      }
    }
    if(dim[0]==*N){
      for(size_t i=0;i<*N;i++)
      for(size_t j=0;j<dim[1];j++){
        VT[j+ldv*i]=U_[j+i*dim[0]];
      }
    }else{
      for(size_t i=0;i<*N;i++)
      for(size_t j=0;j<dim[1];j++){
        VT[j+ldv*i]=V_[i+j*dim[1]];
      }
    }
    for(size_t i=0;i<dim[1];i++){
      S[i]=S[i]*(S[i]<0.0?-1.0:1.0);
    }
    mem::aligned_delete<T>(U_);
    mem::aligned_delete<T>(S_);
    mem::aligned_delete<T>(V_);
  }

  void SVD(Matrix<T>& tU, Matrix<T>& tS, Matrix<T>& tVT){
    pvfmm::Matrix<T>& M=*this;
    pvfmm::Matrix<T> M_=M;
    int n=M.Dim(0);
    int m=M.Dim(1);
    int k = (m<n?m:n);
    tU.Resize(n,k); tU.SetZero();
    tS.Resize(k,k); tS.SetZero();
    tVT.Resize(k,m); tVT.SetZero();
    int INFO=0;
    char JOBU  = 'S';
    char JOBVT = 'S';
    int wssize = 3*(m<n?m:n)+(m>n?m:n);
    int wssize1 = 5*(m<n?m:n);
    wssize = (wssize>wssize1?wssize:wssize1);
    T* wsbuf = mem::aligned_new<T>(wssize);
    svd(&JOBU, &JOBVT, &m, &n, &M[0][0], &m, &tS[0][0], &tVT[0][0], &m, &tU[0][0], &k, wsbuf, &wssize, &INFO);
    mem::aligned_delete<T>(wsbuf);
    if(INFO!=0) std::cout<<INFO<<'\n';
    assert(INFO==0);
    for(size_t i=1;i<k;i++){
    tS[i][i]=tS[0][i];
    tS[0][i]=0;
  }
  }

  Matrix<T> pinv(T eps=-1){
    if(eps<0){
    eps=1.0;
    while(eps+(T)1.0>1.0) eps*=0.5;
    eps=sqrtf(eps);
  }
    Matrix<T> M_r(dim[1],dim[0]);
    tpinv(data_ptr,dim[0],dim[1],eps,M_r.data_ptr);
    this->Resize(0,0);
    return M_r;
  }

};

#define PERM_INT_T size_t
template <class T>
class Permutation{

public:
  Vector<PERM_INT_T> perm;
  Vector<T> scal;

  Permutation(){}

  Permutation(size_t size){
    perm.Resize(size);
    scal.Resize(size);
    for(size_t i=0;i<size;i++){
      perm[i]=i;
      scal[i]=1.0;
    }
  }

  static Permutation<T> RandPerm(size_t size){
    Permutation<T> P(size);
    for(size_t i=0;i<size;i++){
      P.perm[i]=rand()%size;
      for(size_t j=0;j<i;j++)
	if(P.perm[i]==P.perm[j]){ i--; break; }
      P.scal[i]=((T)rand())/RAND_MAX;
    }
    return P;
  }

  Matrix<T> GetMatrix() const{
    size_t size=perm.Dim();
    Matrix<T> M_r(size,size,NULL);
    for(size_t i=0;i<size;i++)
      for(size_t j=0;j<size;j++)
	M_r[i][j]=(perm[j]==i?scal[j]:0.0);
    return M_r;
  }

  size_t Dim() const{
    return perm.Dim();
  }

  Permutation<T> Transpose(){
    size_t size=perm.Dim();
    Permutation<T> P_r(size);
    Vector<PERM_INT_T>& perm_r=P_r.perm;
    Vector<T>& scal_r=P_r.scal;
    for(size_t i=0;i<size;i++){
      perm_r[perm[i]]=i;
      scal_r[perm[i]]=scal[i];
    }
    return P_r;
  }

  Permutation<T> operator*(const Permutation<T>& P){
    size_t size=perm.Dim();
    assert(P.Dim()==size);
    Permutation<T> P_r(size);
    Vector<PERM_INT_T>& perm_r=P_r.perm;
    Vector<T>& scal_r=P_r.scal;
    for(size_t i=0;i<size;i++){
      perm_r[i]=perm[P.perm[i]];
      scal_r[i]=scal[P.perm[i]]*P.scal[i];
    }
    return P_r;
  }

  Matrix<T> operator*(const Matrix<T>& M){
    if(Dim()==0) return M;
    assert(M.Dim(0)==Dim());
    size_t d0=M.Dim(0);
    size_t d1=M.Dim(1);
    Matrix<T> M_r(d0,d1,NULL);
    for(size_t i=0;i<d0;i++){
      const T s=scal[i];
      const T* M_=M[i];
      T* M_r_=M_r[perm[i]];
      for(size_t j=0;j<d1;j++)
	M_r_[j]=M_[j]*s;
    }
    return M_r;
  }

  template <class Y>
  friend Matrix<Y> operator*(const Matrix<Y>& M, const Permutation<Y>& P);

};

template <class T>
Matrix<T> operator*(const Matrix<T>& M, const Permutation<T>& P){
  if(P.Dim()==0) return M;
  assert(M.Dim(1)==P.Dim());
  size_t d0=M.Dim(0);
  size_t d1=M.Dim(1);

  Matrix<T> M_r(d0,d1,NULL);
  for(size_t i=0;i<d0;i++){
    const PERM_INT_T* perm_=&(P.perm[0]);
    const T* scal_=&(P.scal[0]);
    const T* M_=M[i];
    T* M_r_=M_r[i];
    for(size_t j=0;j<d1;j++)
      M_r_[j]=M_[perm_[j]]*scal_[j];
  }
  return M_r;
}

}//end namespace

#endif //_PVFMM_MATRIX_HPP_

#ifndef _PVFMM_FMM_TREE_HPP_
#define _PVFMM_FMM_TREE_HPP_

#include <fmm_pts.hpp>

namespace pvfmm{

template<class Real_t>
inline void matmult_8x8x2(Real_t*& M_, Real_t*& IN0, Real_t*& IN1, Real_t*& OUT0, Real_t*& OUT1){
  Real_t out_reg000, out_reg001, out_reg010, out_reg011;
  Real_t out_reg100, out_reg101, out_reg110, out_reg111;
  Real_t  in_reg000,  in_reg001,  in_reg010,  in_reg011;
  Real_t  in_reg100,  in_reg101,  in_reg110,  in_reg111;
  Real_t   m_reg000,   m_reg001,   m_reg010,   m_reg011;
  Real_t   m_reg100,   m_reg101,   m_reg110,   m_reg111;
  for(int i1=0;i1<8;i1+=2){
    Real_t* IN0_=IN0;
    Real_t* IN1_=IN1;
    out_reg000=OUT0[ 0]; out_reg001=OUT0[ 1];
    out_reg010=OUT0[ 2]; out_reg011=OUT0[ 3];
    out_reg100=OUT1[ 0]; out_reg101=OUT1[ 1];
    out_reg110=OUT1[ 2]; out_reg111=OUT1[ 3];
    for(int i2=0;i2<8;i2+=2){
      m_reg000=M_[ 0]; m_reg001=M_[ 1];
      m_reg010=M_[ 2]; m_reg011=M_[ 3];
      m_reg100=M_[16]; m_reg101=M_[17];
      m_reg110=M_[18]; m_reg111=M_[19];
      in_reg000=IN0_[0]; in_reg001=IN0_[1];
      in_reg010=IN0_[2]; in_reg011=IN0_[3];
      in_reg100=IN1_[0]; in_reg101=IN1_[1];
      in_reg110=IN1_[2]; in_reg111=IN1_[3];
      out_reg000 += m_reg000*in_reg000 - m_reg001*in_reg001;
      out_reg001 += m_reg000*in_reg001 + m_reg001*in_reg000;
      out_reg010 += m_reg010*in_reg000 - m_reg011*in_reg001;
      out_reg011 += m_reg010*in_reg001 + m_reg011*in_reg000;
      out_reg000 += m_reg100*in_reg010 - m_reg101*in_reg011;
      out_reg001 += m_reg100*in_reg011 + m_reg101*in_reg010;
      out_reg010 += m_reg110*in_reg010 - m_reg111*in_reg011;
      out_reg011 += m_reg110*in_reg011 + m_reg111*in_reg010;
      out_reg100 += m_reg000*in_reg100 - m_reg001*in_reg101;
      out_reg101 += m_reg000*in_reg101 + m_reg001*in_reg100;
      out_reg110 += m_reg010*in_reg100 - m_reg011*in_reg101;
      out_reg111 += m_reg010*in_reg101 + m_reg011*in_reg100;
      out_reg100 += m_reg100*in_reg110 - m_reg101*in_reg111;
      out_reg101 += m_reg100*in_reg111 + m_reg101*in_reg110;
      out_reg110 += m_reg110*in_reg110 - m_reg111*in_reg111;
      out_reg111 += m_reg110*in_reg111 + m_reg111*in_reg110;
      M_+=32;
      IN0_+=4;
      IN1_+=4;
    }
    OUT0[ 0]=out_reg000; OUT0[ 1]=out_reg001;
    OUT0[ 2]=out_reg010; OUT0[ 3]=out_reg011;
    OUT1[ 0]=out_reg100; OUT1[ 1]=out_reg101;
    OUT1[ 2]=out_reg110; OUT1[ 3]=out_reg111;
    M_+=4-64*2;
    OUT0+=4;
    OUT1+=4;
  }
}

#if defined(__AVX__) || defined(__SSE3__)
template<>
inline void matmult_8x8x2<double>(double*& M_, double*& IN0, double*& IN1, double*& OUT0, double*& OUT1){
#ifdef __AVX__
  __m256d out00,out01,out10,out11;
  __m256d out20,out21,out30,out31;
  double* in0__ = IN0;
  double* in1__ = IN1;
  out00 = _mm256_load_pd(OUT0);
  out01 = _mm256_load_pd(OUT1);
  out10 = _mm256_load_pd(OUT0+4);
  out11 = _mm256_load_pd(OUT1+4);
  out20 = _mm256_load_pd(OUT0+8);
  out21 = _mm256_load_pd(OUT1+8);
  out30 = _mm256_load_pd(OUT0+12);
  out31 = _mm256_load_pd(OUT1+12);
  for(int i2=0;i2<8;i2+=2){
    __m256d m00;
    __m256d ot00;
    __m256d mt0,mtt0;
    __m256d in00,in00_r,in01,in01_r;
    in00 = _mm256_broadcast_pd((const __m128d*)in0__);
    in00_r = _mm256_permute_pd(in00,5);
    in01 = _mm256_broadcast_pd((const __m128d*)in1__);
    in01_r = _mm256_permute_pd(in01,5);
    m00 = _mm256_load_pd(M_);
    mt0 = _mm256_unpacklo_pd(m00,m00);
    ot00 = _mm256_mul_pd(mt0,in00);
    mtt0 = _mm256_unpackhi_pd(m00,m00);
    out00 = _mm256_add_pd(out00,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in00_r)));
    ot00 = _mm256_mul_pd(mt0,in01);
    out01 = _mm256_add_pd(out01,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in01_r)));
    m00 = _mm256_load_pd(M_+4);
    mt0 = _mm256_unpacklo_pd(m00,m00);
    ot00 = _mm256_mul_pd(mt0,in00);
    mtt0 = _mm256_unpackhi_pd(m00,m00);
    out10 = _mm256_add_pd(out10,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in00_r)));
    ot00 = _mm256_mul_pd(mt0,in01);
    out11 = _mm256_add_pd(out11,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in01_r)));
    m00 = _mm256_load_pd(M_+8);
    mt0 = _mm256_unpacklo_pd(m00,m00);
    ot00 = _mm256_mul_pd(mt0,in00);
    mtt0 = _mm256_unpackhi_pd(m00,m00);
    out20 = _mm256_add_pd(out20,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in00_r)));
    ot00 = _mm256_mul_pd(mt0,in01);
    out21 = _mm256_add_pd(out21,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in01_r)));
    m00 = _mm256_load_pd(M_+12);
    mt0 = _mm256_unpacklo_pd(m00,m00);
    ot00 = _mm256_mul_pd(mt0,in00);
    mtt0 = _mm256_unpackhi_pd(m00,m00);
    out30 = _mm256_add_pd(out30,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in00_r)));
    ot00 = _mm256_mul_pd(mt0,in01);
    out31 = _mm256_add_pd(out31,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in01_r)));
    in00 = _mm256_broadcast_pd((const __m128d*) (in0__+2));
    in00_r = _mm256_permute_pd(in00,5);
    in01 = _mm256_broadcast_pd((const __m128d*) (in1__+2));
    in01_r = _mm256_permute_pd(in01,5);
    m00 = _mm256_load_pd(M_+16);
    mt0 = _mm256_unpacklo_pd(m00,m00);
    ot00 = _mm256_mul_pd(mt0,in00);
    mtt0 = _mm256_unpackhi_pd(m00,m00);
    out00 = _mm256_add_pd(out00,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in00_r)));
    ot00 = _mm256_mul_pd(mt0,in01);
    out01 = _mm256_add_pd(out01,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in01_r)));
    m00 = _mm256_load_pd(M_+20);
    mt0 = _mm256_unpacklo_pd(m00,m00);
    ot00 = _mm256_mul_pd(mt0,in00);
    mtt0 = _mm256_unpackhi_pd(m00,m00);
    out10 = _mm256_add_pd(out10,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in00_r)));
    ot00 = _mm256_mul_pd(mt0,in01);
    out11 = _mm256_add_pd(out11,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in01_r)));
    m00 = _mm256_load_pd(M_+24);
    mt0 = _mm256_unpacklo_pd(m00,m00);
    ot00 = _mm256_mul_pd(mt0,in00);
    mtt0 = _mm256_unpackhi_pd(m00,m00);
    out20 = _mm256_add_pd(out20,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in00_r)));
    ot00 = _mm256_mul_pd(mt0,in01);
    out21 = _mm256_add_pd(out21,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in01_r)));
    m00 = _mm256_load_pd(M_+28);
    mt0 = _mm256_unpacklo_pd(m00,m00);
    ot00 = _mm256_mul_pd(mt0,in00);
    mtt0 = _mm256_unpackhi_pd(m00,m00);
    out30 = _mm256_add_pd(out30,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in00_r)));
    ot00 = _mm256_mul_pd(mt0,in01);
    out31 = _mm256_add_pd(out31,_mm256_addsub_pd(ot00,_mm256_mul_pd(mtt0,in01_r)));
    M_ += 32;
    in0__ += 4;
    in1__ += 4;
  }
  _mm256_store_pd(OUT0,out00);
  _mm256_store_pd(OUT1,out01);
  _mm256_store_pd(OUT0+4,out10);
  _mm256_store_pd(OUT1+4,out11);
  _mm256_store_pd(OUT0+8,out20);
  _mm256_store_pd(OUT1+8,out21);
  _mm256_store_pd(OUT0+12,out30);
  _mm256_store_pd(OUT1+12,out31);
#elif defined __SSE3__
  __m128d out00, out01, out10, out11;
  __m128d in00, in01, in10, in11;
  __m128d m00, m01, m10, m11;
  for(int i1=0;i1<8;i1+=2){
    double* IN0_=IN0;
    double* IN1_=IN1;
    out00 =_mm_load_pd (OUT0  );
    out10 =_mm_load_pd (OUT0+2);
    out01 =_mm_load_pd (OUT1  );
    out11 =_mm_load_pd (OUT1+2);
    for(int i2=0;i2<8;i2+=2){
      m00 =_mm_load1_pd (M_   );
      m10 =_mm_load1_pd (M_+ 2);
      m01 =_mm_load1_pd (M_+16);
      m11 =_mm_load1_pd (M_+18);
      in00 =_mm_load_pd (IN0_  );
      in10 =_mm_load_pd (IN0_+2);
      in01 =_mm_load_pd (IN1_  );
      in11 =_mm_load_pd (IN1_+2);
      out00 = _mm_add_pd   (out00, _mm_mul_pd(m00 , in00 ));
      out00 = _mm_add_pd   (out00, _mm_mul_pd(m01 , in10 ));
      out01 = _mm_add_pd   (out01, _mm_mul_pd(m00 , in01 ));
      out01 = _mm_add_pd   (out01, _mm_mul_pd(m01 , in11 ));
      out10 = _mm_add_pd   (out10, _mm_mul_pd(m10 , in00 ));
      out10 = _mm_add_pd   (out10, _mm_mul_pd(m11 , in10 ));
      out11 = _mm_add_pd   (out11, _mm_mul_pd(m10 , in01 ));
      out11 = _mm_add_pd   (out11, _mm_mul_pd(m11 , in11 ));
      m00 =_mm_load1_pd (M_+   1);
      m10 =_mm_load1_pd (M_+ 2+1);
      m01 =_mm_load1_pd (M_+16+1);
      m11 =_mm_load1_pd (M_+18+1);
      in00 =_mm_shuffle_pd (in00,in00,_MM_SHUFFLE2(0,1));
      in01 =_mm_shuffle_pd (in01,in01,_MM_SHUFFLE2(0,1));
      in10 =_mm_shuffle_pd (in10,in10,_MM_SHUFFLE2(0,1));
      in11 =_mm_shuffle_pd (in11,in11,_MM_SHUFFLE2(0,1));
      out00 = _mm_addsub_pd(out00, _mm_mul_pd(m00, in00));
      out00 = _mm_addsub_pd(out00, _mm_mul_pd(m01, in10));
      out01 = _mm_addsub_pd(out01, _mm_mul_pd(m00, in01));
      out01 = _mm_addsub_pd(out01, _mm_mul_pd(m01, in11));
      out10 = _mm_addsub_pd(out10, _mm_mul_pd(m10, in00));
      out10 = _mm_addsub_pd(out10, _mm_mul_pd(m11, in10));
      out11 = _mm_addsub_pd(out11, _mm_mul_pd(m10, in01));
      out11 = _mm_addsub_pd(out11, _mm_mul_pd(m11, in11));
      M_+=32;
      IN0_+=4;
      IN1_+=4;
    }
    _mm_store_pd (OUT0  ,out00);
    _mm_store_pd (OUT0+2,out10);
    _mm_store_pd (OUT1  ,out01);
    _mm_store_pd (OUT1+2,out11);
    M_+=4-64*2;
    OUT0+=4;
    OUT1+=4;
  }
#endif
}
#endif

#if defined(__SSE3__)
template<>
inline void matmult_8x8x2<float>(float*& M_, float*& IN0, float*& IN1, float*& OUT0, float*& OUT1){
#if defined __SSE3__
  __m128 out00,out01,out10,out11;
  __m128 out20,out21,out30,out31;
  float* in0__ = IN0;
  float* in1__ = IN1;
  out00 = _mm_load_ps(OUT0);
  out01 = _mm_load_ps(OUT1);
  out10 = _mm_load_ps(OUT0+4);
  out11 = _mm_load_ps(OUT1+4);
  out20 = _mm_load_ps(OUT0+8);
  out21 = _mm_load_ps(OUT1+8);
  out30 = _mm_load_ps(OUT0+12);
  out31 = _mm_load_ps(OUT1+12);
  for(int i2=0;i2<8;i2+=2){
    __m128 m00;
    __m128 mt0,mtt0;
    __m128 in00,in00_r,in01,in01_r;
    in00 = _mm_castpd_ps(_mm_load_pd1((const double*)in0__));
    in00_r = _mm_shuffle_ps(in00,in00,_MM_SHUFFLE(2,3,0,1));
    in01 = _mm_castpd_ps(_mm_load_pd1((const double*)in1__));
    in01_r = _mm_shuffle_ps(in01,in01,_MM_SHUFFLE(2,3,0,1));
    m00 = _mm_load_ps(M_);
    mt0  = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(2,2,0,0));
    out00= _mm_add_ps   (out00,_mm_mul_ps( mt0,in00  ));
    mtt0 = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(3,3,1,1));
    out00= _mm_addsub_ps(out00,_mm_mul_ps(mtt0,in00_r));
    out01 = _mm_add_ps   (out01,_mm_mul_ps( mt0,in01  ));
    out01 = _mm_addsub_ps(out01,_mm_mul_ps(mtt0,in01_r));
    m00 = _mm_load_ps(M_+4);
    mt0  = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(2,2,0,0));
    out10= _mm_add_ps   (out10,_mm_mul_ps( mt0,in00  ));
    mtt0 = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(3,3,1,1));
    out10= _mm_addsub_ps(out10,_mm_mul_ps(mtt0,in00_r));
    out11 = _mm_add_ps   (out11,_mm_mul_ps( mt0,in01  ));
    out11 = _mm_addsub_ps(out11,_mm_mul_ps(mtt0,in01_r));
    m00 = _mm_load_ps(M_+8);
    mt0  = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(2,2,0,0));
    out20= _mm_add_ps   (out20,_mm_mul_ps( mt0,in00  ));
    mtt0 = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(3,3,1,1));
    out20= _mm_addsub_ps(out20,_mm_mul_ps(mtt0,in00_r));
    out21 = _mm_add_ps   (out21,_mm_mul_ps( mt0,in01  ));
    out21 = _mm_addsub_ps(out21,_mm_mul_ps(mtt0,in01_r));
    m00 = _mm_load_ps(M_+12);
    mt0  = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(2,2,0,0));
    out30= _mm_add_ps   (out30,_mm_mul_ps( mt0,  in00));
    mtt0 = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(3,3,1,1));
    out30= _mm_addsub_ps(out30,_mm_mul_ps(mtt0,in00_r));
    out31 = _mm_add_ps   (out31,_mm_mul_ps( mt0,in01  ));
    out31 = _mm_addsub_ps(out31,_mm_mul_ps(mtt0,in01_r));
    in00 = _mm_castpd_ps(_mm_load_pd1((const double*) (in0__+2)));
    in00_r = _mm_shuffle_ps(in00,in00,_MM_SHUFFLE(2,3,0,1));
    in01 = _mm_castpd_ps(_mm_load_pd1((const double*) (in1__+2)));
    in01_r = _mm_shuffle_ps(in01,in01,_MM_SHUFFLE(2,3,0,1));
    m00 = _mm_load_ps(M_+16);
    mt0  = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(2,2,0,0));
    out00= _mm_add_ps   (out00,_mm_mul_ps( mt0,in00  ));
    mtt0 = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(3,3,1,1));
    out00= _mm_addsub_ps(out00,_mm_mul_ps(mtt0,in00_r));
    out01 = _mm_add_ps   (out01,_mm_mul_ps( mt0,in01  ));
    out01 = _mm_addsub_ps(out01,_mm_mul_ps(mtt0,in01_r));
    m00 = _mm_load_ps(M_+20);
    mt0  = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(2,2,0,0));
    out10= _mm_add_ps   (out10,_mm_mul_ps( mt0,in00  ));
    mtt0 = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(3,3,1,1));
    out10= _mm_addsub_ps(out10,_mm_mul_ps(mtt0,in00_r));
    out11 = _mm_add_ps   (out11,_mm_mul_ps( mt0,in01 ));
    out11 = _mm_addsub_ps(out11,_mm_mul_ps(mtt0,in01_r));
    m00 = _mm_load_ps(M_+24);
    mt0  = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(2,2,0,0));
    out20= _mm_add_ps   (out20,_mm_mul_ps( mt0,in00  ));
    mtt0 = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(3,3,1,1));
    out20= _mm_addsub_ps(out20,_mm_mul_ps(mtt0,in00_r));
    out21 = _mm_add_ps   (out21,_mm_mul_ps( mt0,in01  ));
    out21 = _mm_addsub_ps(out21,_mm_mul_ps(mtt0,in01_r));
    m00 = _mm_load_ps(M_+28);
    mt0  = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(2,2,0,0));
    out30= _mm_add_ps   (out30,_mm_mul_ps( mt0,in00  ));
    mtt0 = _mm_shuffle_ps(m00,m00,_MM_SHUFFLE(3,3,1,1));
    out30= _mm_addsub_ps(out30,_mm_mul_ps(mtt0,in00_r));
    out31 = _mm_add_ps   (out31,_mm_mul_ps( mt0,in01  ));
    out31 = _mm_addsub_ps(out31,_mm_mul_ps(mtt0,in01_r));
    M_ += 32;
    in0__ += 4;
    in1__ += 4;
  }
  _mm_store_ps(OUT0,out00);
  _mm_store_ps(OUT1,out01);
  _mm_store_ps(OUT0+4,out10);
  _mm_store_ps(OUT1+4,out11);
  _mm_store_ps(OUT0+8,out20);
  _mm_store_ps(OUT1+8,out21);
  _mm_store_ps(OUT0+12,out30);
  _mm_store_ps(OUT1+12,out31);
#endif
}
#endif

template <class FMMNode_t>
class FMM_Tree : public FMM_Pts<FMMNode_t> {

 public:
  struct PackedData{
    size_t len;
    Matrix<Real_t>* ptr;
    Vector<size_t> cnt;
    Vector<size_t> dsp;
  };
  struct InteracData{
    Vector<size_t> in_node;
    Vector<size_t> scal_idx;
    Vector<Real_t> coord_shift;
    Vector<size_t> interac_cnt;
    Vector<size_t> interac_dsp;
    Vector<size_t> interac_cst;
    Vector<Real_t> scal[4*MAX_DEPTH];
    Matrix<Real_t> M[4];
  };
  struct ptSetupData{
    int level;
    const Kernel<Real_t>* kernel;
    PackedData src_coord;
    PackedData src_value;
    PackedData srf_coord;
    PackedData srf_value;
    PackedData trg_coord;
    PackedData trg_value;
    InteracData interac_data;
  };

 private:

  inline int p2oLocal(Vector<MortonId> & nodes, Vector<MortonId>& leaves,
		      unsigned int maxNumPts, unsigned int maxDepth, bool complete) {
    assert(maxDepth<=MAX_DEPTH);
    std::vector<MortonId> leaves_lst;
    unsigned int init_size=leaves.Dim();
    unsigned int num_pts=nodes.Dim();
    MortonId curr_node=leaves[0];
    MortonId last_node=leaves[init_size-1].NextId();
    MortonId next_node;
    unsigned int curr_pt=0;
    unsigned int next_pt=curr_pt+maxNumPts;
    while(next_pt <= num_pts){
      next_node = curr_node.NextId();
      while( next_pt < num_pts && next_node > nodes[next_pt] && curr_node.GetDepth() < maxDepth-1 ){
	curr_node = curr_node.getDFD(curr_node.GetDepth()+1);
	next_node = curr_node.NextId();
      }
      leaves_lst.push_back(curr_node);
      curr_node = next_node;
      unsigned int inc=maxNumPts;
      while(next_pt < num_pts && curr_node > nodes[next_pt]){
	inc=inc<<1;
	next_pt+=inc;
	if(next_pt > num_pts){
	  next_pt = num_pts;
	  break;
	}
      }
      curr_pt = std::lower_bound(&nodes[0]+curr_pt,&nodes[0]+next_pt,curr_node,std::less<MortonId>())-&nodes[0];
      if(curr_pt >= num_pts) break;
      next_pt = curr_pt + maxNumPts;
      if(next_pt > num_pts) next_pt = num_pts;
    }
#ifndef NDEBUG
    for(size_t i=0;i<leaves_lst.size();i++){
      size_t a=std::lower_bound(&nodes[0],&nodes[0]+nodes.Dim(),leaves_lst[i],std::less<MortonId>())-&nodes[0];
      size_t b=std::lower_bound(&nodes[0],&nodes[0]+nodes.Dim(),leaves_lst[i].NextId(),std::less<MortonId>())-&nodes[0];
      assert(b-a<=maxNumPts || leaves_lst[i].GetDepth()==maxDepth-1);
      if(i==leaves_lst.size()-1) assert(b==nodes.Dim() && a<nodes.Dim());
      if(i==0) assert(a==0);
    }
#endif
    if(complete) {
      while(curr_node<last_node){
	while( curr_node.NextId() > last_node && curr_node.GetDepth() < maxDepth-1 )
	  curr_node = curr_node.getDFD(curr_node.GetDepth()+1);
	leaves_lst.push_back(curr_node);
	curr_node = curr_node.NextId();
      }
    }
    leaves=leaves_lst;
    return 0;
  }

  inline int points2Octree(const Vector<MortonId>& pt_mid, Vector<MortonId>& nodes,
			   unsigned int maxDepth, unsigned int maxNumPts) {
    int myrank=0, np=1;
    Profile::Tic("SortMortonId", true, 10);
    Vector<MortonId> pt_sorted;
    par::HyperQuickSort(pt_mid, pt_sorted);
    size_t pt_cnt=pt_sorted.Dim();
    Profile::Toc();

    Profile::Tic("p2o_local", false, 10);
    Vector<MortonId> nodes_local(1); nodes_local[0]=MortonId();
    p2oLocal(pt_sorted, nodes_local, maxNumPts, maxDepth, myrank==np-1);
    Profile::Toc();

    Profile::Tic("RemoveDuplicates", true, 10);
    {
      size_t node_cnt=nodes_local.Dim();
      MortonId first_node;
      MortonId  last_node=nodes_local[node_cnt-1];
      size_t i=0;
      std::vector<MortonId> node_lst;
      if(myrank){
	while(i<node_cnt && nodes_local[i].getDFD(maxDepth)<first_node) i++; assert(i);
	last_node=nodes_local[i>0?i-1:0].NextId();

	while(first_node<last_node){
	  while(first_node.isAncestor(last_node))
	    first_node=first_node.getDFD(first_node.GetDepth()+1);
	  if(first_node==last_node) break;
	  node_lst.push_back(first_node);
	  first_node=first_node.NextId();
	}
      }
      for(;i<node_cnt-(myrank==np-1?0:1);i++) node_lst.push_back(nodes_local[i]);
      nodes=node_lst;
    }
    Profile::Toc();

    return 0;
  }

  template <class Real_t>
  void VListHadamard(size_t dof, size_t M_dim, size_t ker_dim0, size_t ker_dim1, Vector<size_t>& interac_dsp,
      Vector<size_t>& interac_vec, Vector<Real_t*>& precomp_mat, Vector<Real_t>& fft_in, Vector<Real_t>& fft_out){
    size_t chld_cnt=1UL<<COORD_DIM;
    size_t fftsize_in =M_dim*ker_dim0*chld_cnt*2;
    size_t fftsize_out=M_dim*ker_dim1*chld_cnt*2;
    Real_t* zero_vec0=mem::aligned_new<Real_t>(fftsize_in );
    Real_t* zero_vec1=mem::aligned_new<Real_t>(fftsize_out);
    size_t n_out=fft_out.Dim()/fftsize_out;
#pragma omp parallel for
    for(size_t k=0;k<n_out;k++){
      Vector<Real_t> dnward_check_fft(fftsize_out, &fft_out[k*fftsize_out], false);
      dnward_check_fft.SetZero();
    }
    size_t mat_cnt=precomp_mat.Dim();
    size_t blk1_cnt=interac_dsp.Dim()/mat_cnt;
    const size_t V_BLK_SIZE=V_BLK_CACHE*64/sizeof(Real_t);
    Real_t** IN_ =mem::aligned_new<Real_t*>(2*V_BLK_SIZE*blk1_cnt*mat_cnt);
    Real_t** OUT_=mem::aligned_new<Real_t*>(2*V_BLK_SIZE*blk1_cnt*mat_cnt);
#pragma omp parallel for
    for(size_t interac_blk1=0; interac_blk1<blk1_cnt*mat_cnt; interac_blk1++){
      size_t interac_dsp0 = (interac_blk1==0?0:interac_dsp[interac_blk1-1]);
      size_t interac_dsp1 =                    interac_dsp[interac_blk1  ] ;
      size_t interac_cnt  = interac_dsp1-interac_dsp0;
      for(size_t j=0;j<interac_cnt;j++){
        IN_ [2*V_BLK_SIZE*interac_blk1 +j]=&fft_in [interac_vec[(interac_dsp0+j)*2+0]];
        OUT_[2*V_BLK_SIZE*interac_blk1 +j]=&fft_out[interac_vec[(interac_dsp0+j)*2+1]];
      }
      IN_ [2*V_BLK_SIZE*interac_blk1 +interac_cnt]=zero_vec0;
      OUT_[2*V_BLK_SIZE*interac_blk1 +interac_cnt]=zero_vec1;
    }
    int omp_p=omp_get_max_threads();
#pragma omp parallel for
    for(int pid=0; pid<omp_p; pid++){
      size_t a=( pid   *M_dim)/omp_p;
      size_t b=((pid+1)*M_dim)/omp_p;
      for(int in_dim=0;in_dim<ker_dim0;in_dim++)
      for(int ot_dim=0;ot_dim<ker_dim1;ot_dim++)
      for(size_t     blk1=0;     blk1<blk1_cnt;    blk1++)
      for(size_t        k=a;        k<       b;       k++)
      for(size_t mat_indx=0; mat_indx< mat_cnt;mat_indx++){
        size_t interac_blk1 = blk1*mat_cnt+mat_indx;
        size_t interac_dsp0 = (interac_blk1==0?0:interac_dsp[interac_blk1-1]);
        size_t interac_dsp1 =                    interac_dsp[interac_blk1  ] ;
        size_t interac_cnt  = interac_dsp1-interac_dsp0;
        Real_t** IN = IN_ + 2*V_BLK_SIZE*interac_blk1;
        Real_t** OUT= OUT_+ 2*V_BLK_SIZE*interac_blk1;
        Real_t* M = precomp_mat[mat_indx] + k*chld_cnt*chld_cnt*2 + (ot_dim+in_dim*ker_dim1)*M_dim*128;
        {
          for(size_t j=0;j<interac_cnt;j+=2){
            Real_t* M_   = M;
            Real_t* IN0  = IN [j+0] + (in_dim*M_dim+k)*chld_cnt*2;
            Real_t* IN1  = IN [j+1] + (in_dim*M_dim+k)*chld_cnt*2;
            Real_t* OUT0 = OUT[j+0] + (ot_dim*M_dim+k)*chld_cnt*2;
            Real_t* OUT1 = OUT[j+1] + (ot_dim*M_dim+k)*chld_cnt*2;
#ifdef __SSE__
            if (j+2 < interac_cnt) {
              _mm_prefetch(((char *)(IN[j+2] + (in_dim*M_dim+k)*chld_cnt*2)), _MM_HINT_T0);
              _mm_prefetch(((char *)(IN[j+2] + (in_dim*M_dim+k)*chld_cnt*2) + 64), _MM_HINT_T0);
              _mm_prefetch(((char *)(IN[j+3] + (in_dim*M_dim+k)*chld_cnt*2)), _MM_HINT_T0);
              _mm_prefetch(((char *)(IN[j+3] + (in_dim*M_dim+k)*chld_cnt*2) + 64), _MM_HINT_T0);
              _mm_prefetch(((char *)(OUT[j+2] + (ot_dim*M_dim+k)*chld_cnt*2)), _MM_HINT_T0);
              _mm_prefetch(((char *)(OUT[j+2] + (ot_dim*M_dim+k)*chld_cnt*2) + 64), _MM_HINT_T0);
              _mm_prefetch(((char *)(OUT[j+3] + (ot_dim*M_dim+k)*chld_cnt*2)), _MM_HINT_T0);
              _mm_prefetch(((char *)(OUT[j+3] + (ot_dim*M_dim+k)*chld_cnt*2) + 64), _MM_HINT_T0);
            }
#endif
            matmult_8x8x2(M_, IN0, IN1, OUT0, OUT1);
          }
        }
      }
    }
    {
      Profile::Add_FLOP(8*8*8*(interac_vec.Dim()/2)*M_dim*ker_dim0*ker_dim1*dof);
    }
    mem::aligned_delete<Real_t*>(IN_ );
    mem::aligned_delete<Real_t*>(OUT_);
    mem::aligned_delete<Real_t>(zero_vec0);
    mem::aligned_delete<Real_t>(zero_vec1);
  }

  template<typename ElemType>
  void CopyVec(std::vector<std::vector<ElemType> >& vec_, pvfmm::Vector<ElemType>& vec) {
    int omp_p=omp_get_max_threads();
    std::vector<size_t> vec_dsp(omp_p+1,0);
    for(size_t tid=0;tid<omp_p;tid++){
      vec_dsp[tid+1]=vec_dsp[tid]+vec_[tid].size();
    }
    vec.ReInit(vec_dsp[omp_p]);
#pragma omp parallel for
    for(size_t tid=0;tid<omp_p;tid++){
      memcpy(&vec[0]+vec_dsp[tid],&vec_[tid][0],vec_[tid].size()*sizeof(ElemType));
    }
  }

 public:

  typedef typename FMM_Pts<FMMNode_t>::FMMData FMMData_t;
  typedef FMM_Tree FMMTree_t;
  using FMM_Pts<FMMNode_t>::kernel;
  using FMM_Pts<FMMNode_t>::surface;
  using FMM_Pts<FMMNode_t>::Precomp;
  using FMM_Pts<FMMNode_t>::MultipoleOrder;
  using FMM_Pts<FMMNode_t>::SetupInterac;
  using FMM_Pts<FMMNode_t>::vlist_fft_flag;
  using FMM_Pts<FMMNode_t>::vlist_ifft_flag;
  using FMM_Pts<FMMNode_t>::vlist_fftplan;
  using FMM_Pts<FMMNode_t>::vlist_ifftplan;

  int dim;
  FMMNode_t* root_node;
  int max_depth;
  std::vector<FMMNode_t*> node_lst;
  mem::MemoryManager memgr;

  std::vector<Matrix<Real_t> > node_data_buff;
  pvfmm::Matrix<FMMNode_t*> node_interac_lst;
  InteracList<FMMNode_t> interac_list;
  BoundaryType bndry;
  std::vector<Matrix<char> > precomp_lst;
  std::vector<SetupData<Real_t,FMMNode_t> > setup_data;
  std::vector<Vector<Real_t> > upwd_check_surf;
  std::vector<Vector<Real_t> > upwd_equiv_surf;
  std::vector<Vector<Real_t> > dnwd_check_surf;
  std::vector<Vector<Real_t> > dnwd_equiv_surf;

  FMM_Tree(): dim(0), root_node(NULL), max_depth(MAX_DEPTH), memgr(0), bndry(FreeSpace) { };

  ~FMM_Tree(){
    if(RootNode()!=NULL){
      mem::aligned_delete(root_node);
    }
  }

  void Initialize(typename FMM_Node::NodeData* init_data) {
    Profile::Tic("InitTree",true);{
      Profile::Tic("InitRoot",false,5);
      dim=init_data->dim;
      max_depth=init_data->max_depth;
      if(max_depth>MAX_DEPTH) max_depth=MAX_DEPTH;
      if(root_node) mem::aligned_delete(root_node);
      root_node=mem::aligned_new<FMMNode_t>();
      root_node->Initialize(NULL,0,init_data);
      FMMNode_t* rnode=this->RootNode();
      assert(this->dim==COORD_DIM);
      Profile::Toc();
  
      Profile::Tic("Points2Octee",true,5);
      Vector<MortonId> lin_oct;
      {
        Vector<MortonId> pt_mid;
        Vector<Real_t>& pt_c=rnode->pt_coord;
        size_t pt_cnt=pt_c.Dim()/this->dim;
        pt_mid.Resize(pt_cnt);
#pragma omp parallel for
        for(size_t i=0;i<pt_cnt;i++){
        pt_mid[i]=MortonId(pt_c[i*COORD_DIM+0],pt_c[i*COORD_DIM+1],pt_c[i*COORD_DIM+2],this->max_depth);
      }
        points2Octree(pt_mid,lin_oct,this->max_depth,init_data->max_pts);
      }
      Profile::Toc();
  
      Profile::Tic("ScatterPoints",true,5);
      {
        std::vector<Vector<Real_t>*> coord_lst;
        std::vector<Vector<Real_t>*> value_lst;
        std::vector<Vector<size_t>*> scatter_lst;
        rnode->NodeDataVec(coord_lst, value_lst, scatter_lst);
        assert(coord_lst.size()==value_lst.size());
        assert(coord_lst.size()==scatter_lst.size());
  
        Vector<MortonId> pt_mid;
        Vector<size_t> scatter_index;
        for(size_t i=0;i<coord_lst.size();i++){
          if(!coord_lst[i]) continue;
          Vector<Real_t>& pt_c=*coord_lst[i];
          size_t pt_cnt=pt_c.Dim()/this->dim;
          pt_mid.Resize(pt_cnt);
#pragma omp parallel for
          for(size_t i=0;i<pt_cnt;i++){
    	  pt_mid[i]=MortonId(pt_c[i*COORD_DIM+0],pt_c[i*COORD_DIM+1],pt_c[i*COORD_DIM+2],this->max_depth);
          }
          par::SortScatterIndex(pt_mid  , scatter_index, &lin_oct[0]);
          par::ScatterForward  (pt_c, scatter_index);
          if(value_lst[i]!=NULL){
            Vector<Real_t>& pt_v=*value_lst[i];
            par::ScatterForward(pt_v, scatter_index);
          }
          if(scatter_lst[i]!=NULL){
            Vector<size_t>& pt_s=*scatter_lst[i];
            pt_s=scatter_index;
          }
        }
      }
      Profile::Toc();
  
      Profile::Tic("PointerTree",false,5);
      {
        int omp_p=omp_get_max_threads();
  
        rnode->SetGhost(false);
        for(int i=0;i<omp_p;i++){
          size_t idx=(lin_oct.Dim()*i)/omp_p;
          FMMNode_t* n=FindNode(lin_oct[idx], true);
          assert(n->GetMortonId()==lin_oct[idx]);
          UNUSED(n);
        }
  
#pragma omp parallel for
        for(int i=0;i<omp_p;i++){
          size_t a=(lin_oct.Dim()* i   )/omp_p;
          size_t b=(lin_oct.Dim()*(i+1))/omp_p;
  
          size_t idx=a;
          FMMNode_t* n=FindNode(lin_oct[idx], false);
          if(a==0) n=rnode;
          while(n!=NULL && (idx<b || i==omp_p-1)){
            n->SetGhost(false);
            MortonId dn=n->GetMortonId();
            if(idx<b && dn.isAncestor(lin_oct[idx])){
              if(n->IsLeaf()) n->Subdivide();
            }else if(idx<b && dn==lin_oct[idx]){
              if(!n->IsLeaf()) n->Truncate();
              assert(n->IsLeaf());
              idx++;
            }else{
              n->Truncate();
              n->SetGhost(true);
            }
            n=this->PreorderNxt(n);
          }
        }
      }Profile::Toc();
      Profile::Tic("InitFMMData",true,5);{
	std::vector<FMMNode_t*>& nodes=this->GetNodeList();
#pragma omp parallel for
	for(size_t i=0;i<nodes.size();i++){
	  if(nodes[i]->FMMData()==NULL) nodes[i]->FMMData()=mem::aligned_new<FMMData_t>();
	}
      }Profile::Toc();   
    }Profile::Toc();
  }

  int Dim() {return dim;}

  FMMNode_t* RootNode() {return root_node;}

  FMMNode_t* PreorderFirst() {return root_node;}

  FMMNode_t* PreorderNxt(FMMNode_t* curr_node) {
    assert(curr_node!=NULL);
    int n=(1UL<<dim);
    if(!curr_node->IsLeaf())
      for(int i=0;i<n;i++)
	if(curr_node->Child(i)!=NULL)
	  return curr_node->Child(i);
    FMMNode_t* node=curr_node;
    while(true){
      int i=node->Path2Node()+1;
      node=node->Parent();
      if(node==NULL) return NULL;
      for(;i<n;i++)
	if(node->Child(i)!=NULL)
	  return node->Child(i);
    }
  }

  void SetColleagues(BoundaryType bndry=FreeSpace, FMMNode_t* node=NULL) {
    int n1=(int)pvfmm::pow<unsigned int>(3,this->Dim());
    int n2=(int)pvfmm::pow<unsigned int>(2,this->Dim());
    if(node==NULL){
      FMMNode_t* curr_node=this->PreorderFirst();
      if(curr_node!=NULL){
        if(bndry==Periodic){
          for(int i=0;i<n1;i++)
            curr_node->SetColleague(curr_node,i);
        }else{
          curr_node->SetColleague(curr_node,(n1-1)/2);
        }
        curr_node=this->PreorderNxt(curr_node);
      }
      Vector<std::vector<FMMNode_t*> > nodes(MAX_DEPTH);
      while(curr_node!=NULL){
        nodes[curr_node->depth].push_back(curr_node);
        curr_node=this->PreorderNxt(curr_node);
      }
      for(size_t i=0;i<MAX_DEPTH;i++){
        size_t j0=nodes[i].size();
        FMMNode_t** nodes_=&nodes[i][0];
#pragma omp parallel for
        for(size_t j=0;j<j0;j++){
          SetColleagues(bndry, nodes_[j]);
        }
      }
    }else{
      FMMNode_t* parent_node;
      FMMNode_t* tmp_node1;
      FMMNode_t* tmp_node2;
      for(int i=0;i<n1;i++)node->SetColleague(NULL,i);
      parent_node=node->Parent();
      if(parent_node==NULL) return;
      int l=node->Path2Node();
      for(int i=0;i<n1;i++){
        tmp_node1=parent_node->Colleague(i);
        if(tmp_node1!=NULL)
        if(!tmp_node1->IsLeaf()){
          for(int j=0;j<n2;j++){
            tmp_node2=tmp_node1->Child(j);
            if(tmp_node2!=NULL){

              bool flag=true;
              int a=1,b=1,new_indx=0;
              for(int k=0;k<this->Dim();k++){
                int indx_diff=(((i/b)%3)-1)*2+((j/a)%2)-((l/a)%2);
                if(-1>indx_diff || indx_diff>1) flag=false;
                new_indx+=(indx_diff+1)*b;
                a*=2;b*=3;
              }
              if(flag){
                node->SetColleague(tmp_node2,new_indx);
              }
            }
          }
        }
      }
    }
  }

  std::vector<FMMNode_t*>& GetNodeList() {
    if(root_node->GetStatus() & 1){
      node_lst.clear();
      FMMNode_t* n=this->PreorderFirst();
      while(n!=NULL){
	int& status=n->GetStatus();
	status=(status & (~(int)1));
	node_lst.push_back(n);
	n=this->PreorderNxt(n);
      }
    }
    return node_lst;
  }

  FMMNode_t* FindNode(MortonId& key, bool subdiv, FMMNode_t* start=NULL) {
    int num_child=1UL<<this->Dim();
    FMMNode_t* n=start;
    if(n==NULL) n=this->RootNode();
    while(n->GetMortonId()<key && (!n->IsLeaf()||subdiv)){
      if(n->IsLeaf() && !n->IsGhost()) n->Subdivide();
      if(n->IsLeaf()) break;
      for(int j=0;j<num_child;j++){
	if(n->Child(j)->GetMortonId().NextId()>key){
	  n=n->Child(j);
	  break;
	}
      }
    }
    assert(!subdiv || n->IsGhost() || n->GetMortonId()==key);
    return n;
  }

  FMMNode_t* PostorderFirst() {
    FMMNode_t* node=root_node;
    int n=(1UL<<dim);
    while(true){
      if(node->IsLeaf()) return node;
      for(int i=0;i<n;i++) {
	if(node->Child(i)!=NULL){
	  node=node->Child(i);
	  break;
	}
      }
    }
  }

  FMMNode_t* PostorderNxt(FMMNode_t* curr_node) {
    assert(curr_node!=NULL);
    FMMNode_t* node=curr_node;
    int j=node->Path2Node()+1;
    node=node->Parent();
    if(node==NULL) return NULL;
    int n=(1UL<<dim);
    for(;j<n;j++){
      if(node->Child(j)!=NULL){
	node=node->Child(j);
	while(true){
	  if(node->IsLeaf()) return node;
	  for(int i=0;i<n;i++) {
	    if(node->Child(i)!=NULL){
	      node=node->Child(i);
	      break;
	    }
	  }
	}
      }
    }
    return node;
  }

  void InitFMM_Tree(bool refine, BoundaryType bndry_=FreeSpace) {
    Profile::Tic("InitFMM_Tree",true);{
      interac_list.Initialize(this->Dim());
      bndry=bndry_;
    }Profile::Toc();
  }

  void SetupFMM() {
    Profile::Tic("SetupFMM",true);{
    Profile::Tic("SetColleagues",false,3);
    this->SetColleagues(bndry);
    Profile::Toc();
    Profile::Tic("CollectNodeData",false,3);
    FMMNode_t* n=dynamic_cast<FMMNode_t*>(this->PostorderFirst());
    std::vector<FMMNode_t*> all_nodes;
    while(n!=NULL){
      n->pt_cnt[0]=0;
      n->pt_cnt[1]=0;
      all_nodes.push_back(n);
      n=static_cast<FMMNode_t*>(this->PostorderNxt(n));
    }
    std::vector<Vector<FMMNode_t*> > node_lists; // TODO: Remove this parameter, not really needed
    this->CollectNodeData(this,all_nodes, node_data_buff, node_lists);
    Profile::Toc();
  
    Profile::Tic("BuildLists",false,3);
    BuildInteracLists();
    Profile::Toc();
    setup_data.resize(8*MAX_DEPTH);
    precomp_lst.resize(8);
    Profile::Tic("UListSetup",false,3);
    for(size_t i=0;i<MAX_DEPTH;i++){
      setup_data[i+MAX_DEPTH*0].precomp_data=&precomp_lst[0];
      this->U_ListSetup(setup_data[i+MAX_DEPTH*0],this,node_data_buff,node_lists,this->ScaleInvar()?(i==0?-1:MAX_DEPTH+1):i);
    }
    Profile::Toc();
    Profile::Tic("WListSetup",false,3);
    for(size_t i=0;i<MAX_DEPTH;i++){
      setup_data[i+MAX_DEPTH*1].precomp_data=&precomp_lst[1];
      this->W_ListSetup(setup_data[i+MAX_DEPTH*1],this,node_data_buff,node_lists,this->ScaleInvar()?(i==0?-1:MAX_DEPTH+1):i);
    }
    Profile::Toc();
    Profile::Tic("XListSetup",false,3);
    for(size_t i=0;i<MAX_DEPTH;i++){
      setup_data[i+MAX_DEPTH*2].precomp_data=&precomp_lst[2];
      this->X_ListSetup(setup_data[i+MAX_DEPTH*2],this,node_data_buff,node_lists,this->ScaleInvar()?(i==0?-1:MAX_DEPTH+1):i);
    }
    Profile::Toc();
    Profile::Tic("VListSetup",false,3);
    for(size_t i=0;i<MAX_DEPTH;i++){
      setup_data[i+MAX_DEPTH*3].precomp_data=&precomp_lst[3];
      this->V_ListSetup(setup_data[i+MAX_DEPTH*3],this,node_data_buff,node_lists,this->ScaleInvar()?(i==0?-1:MAX_DEPTH+1):i);
    }
    Profile::Toc();
    Profile::Tic("D2DSetup",false,3);
    for(size_t i=0;i<MAX_DEPTH;i++){
      setup_data[i+MAX_DEPTH*4].precomp_data=&precomp_lst[4];
      this->Down2DownSetup(setup_data[i+MAX_DEPTH*4],this,node_data_buff,node_lists,i);
    }
    Profile::Toc();
    Profile::Tic("D2TSetup",false,3);
    for(size_t i=0;i<MAX_DEPTH;i++){
      setup_data[i+MAX_DEPTH*5].precomp_data=&precomp_lst[5];
      this->Down2TargetSetup(setup_data[i+MAX_DEPTH*5],this,node_data_buff,node_lists,this->ScaleInvar()?(i==0?-1:MAX_DEPTH+1):i);
    }
    Profile::Toc();
  
    Profile::Tic("S2USetup",false,3);
    for(size_t i=0;i<MAX_DEPTH;i++){
      setup_data[i+MAX_DEPTH*6].precomp_data=&precomp_lst[6];
      this->Source2UpSetup(setup_data[i+MAX_DEPTH*6],this,node_data_buff,node_lists,this->ScaleInvar()?(i==0?-1:MAX_DEPTH+1):i);
    }
    Profile::Toc();
    Profile::Tic("U2USetup",false,3);
    for(size_t i=0;i<MAX_DEPTH;i++){
      setup_data[i+MAX_DEPTH*7].precomp_data=&precomp_lst[7];
      this->Up2UpSetup(setup_data[i+MAX_DEPTH*7],this,node_data_buff,node_lists,i);
    }
    Profile::Toc();
    ClearFMMData();
    }Profile::Toc();
  }
  
  void ClearFMMData() {
  Profile::Tic("ClearFMMData",true);{
    int omp_p=omp_get_max_threads();
#pragma omp parallel for
    for(int j=0;j<omp_p;j++){
      Matrix<Real_t>* mat;
      mat=setup_data[0+MAX_DEPTH*1]. input_data;
      if(mat && mat->Dim(0)*mat->Dim(1)){
        size_t a=(mat->Dim(0)*mat->Dim(1)*(j+0))/omp_p;
        size_t b=(mat->Dim(0)*mat->Dim(1)*(j+1))/omp_p;
        memset(&(*mat)[0][a],0,(b-a)*sizeof(Real_t));
      }
      mat=setup_data[0+MAX_DEPTH*2].output_data;
      if(mat && mat->Dim(0)*mat->Dim(1)){
        size_t a=(mat->Dim(0)*mat->Dim(1)*(j+0))/omp_p;
        size_t b=(mat->Dim(0)*mat->Dim(1)*(j+1))/omp_p;
        memset(&(*mat)[0][a],0,(b-a)*sizeof(Real_t));
      }
      mat=setup_data[0+MAX_DEPTH*0].output_data;
      if(mat && mat->Dim(0)*mat->Dim(1)){
        size_t a=(mat->Dim(0)*mat->Dim(1)*(j+0))/omp_p;
        size_t b=(mat->Dim(0)*mat->Dim(1)*(j+1))/omp_p;
        memset(&(*mat)[0][a],0,(b-a)*sizeof(Real_t));
      }
    }
  }Profile::Toc();
  }
  
  void RunFMM() {
    Profile::Tic("RunFMM",true);
    {
      Profile::Tic("UpwardPass",false,2);
      UpwardPass();
      Profile::Toc();
      Profile::Tic("DownwardPass",true,2);
      DownwardPass();
      Profile::Toc();
    }
    Profile::Toc();
  }

  void EvalList(SetupData<Real_t,FMMNode_t>& setup_data){
    if(setup_data.interac_data.Dim(0)==0 || setup_data.interac_data.Dim(1)==0){
      return;
    }
    Profile::Tic("Host2Device",false,25);
    typename Vector<char>::Device          buff;
    typename Matrix<char>::Device  precomp_data;
    typename Matrix<char>::Device  interac_data;
    typename Matrix<Real_t>::Device  input_data;
    typename Matrix<Real_t>::Device output_data;
    buff        =       this-> dev_buffer;
    precomp_data=*setup_data.precomp_data;
    interac_data= setup_data.interac_data;
    input_data  =*setup_data.  input_data;
    output_data =*setup_data. output_data;
    Profile::Toc();
    Profile::Tic("DeviceComp",false,20);
    int lock_idx=-1;
    int wait_lock_idx=-1;
    {
      size_t data_size, M_dim0, M_dim1, dof;
      Vector<size_t> interac_blk;
      Vector<size_t> interac_cnt;
      Vector<size_t> interac_mat;
      Vector<size_t>  input_perm;
      Vector<size_t> output_perm;
      {
        char* data_ptr=&interac_data[0][0];
        data_size=((size_t*)data_ptr)[0]; data_ptr+=data_size;
        data_size=((size_t*)data_ptr)[0]; data_ptr+=sizeof(size_t);
        M_dim0   =((size_t*)data_ptr)[0]; data_ptr+=sizeof(size_t);
        M_dim1   =((size_t*)data_ptr)[0]; data_ptr+=sizeof(size_t);
        dof      =((size_t*)data_ptr)[0]; data_ptr+=sizeof(size_t);
        interac_blk.ReInit(((size_t*)data_ptr)[0],(size_t*)(data_ptr+sizeof(size_t)),false);
        data_ptr+=sizeof(size_t)+interac_blk.Dim()*sizeof(size_t);
        interac_cnt.ReInit(((size_t*)data_ptr)[0],(size_t*)(data_ptr+sizeof(size_t)),false);
        data_ptr+=sizeof(size_t)+interac_cnt.Dim()*sizeof(size_t);
        interac_mat.ReInit(((size_t*)data_ptr)[0],(size_t*)(data_ptr+sizeof(size_t)),false);
        data_ptr+=sizeof(size_t)+interac_mat.Dim()*sizeof(size_t);
        input_perm .ReInit(((size_t*)data_ptr)[0],(size_t*)(data_ptr+sizeof(size_t)),false);
        data_ptr+=sizeof(size_t)+ input_perm.Dim()*sizeof(size_t);
        output_perm.ReInit(((size_t*)data_ptr)[0],(size_t*)(data_ptr+sizeof(size_t)),false);
        data_ptr+=sizeof(size_t)+output_perm.Dim()*sizeof(size_t);
      }
      {
        int omp_p=omp_get_max_threads();
        size_t interac_indx=0;
        size_t interac_blk_dsp=0;
        for(size_t k=0;k<interac_blk.Dim();k++){
          size_t vec_cnt=0;
          for(size_t j=interac_blk_dsp;j<interac_blk_dsp+interac_blk[k];j++) vec_cnt+=interac_cnt[j];
          if(vec_cnt==0){
            interac_blk_dsp += interac_blk[k];
            continue;
          }
          char* buff_in =&buff[0];
          char* buff_out=&buff[vec_cnt*dof*M_dim0*sizeof(Real_t)];
#pragma omp parallel for
          for(int tid=0;tid<omp_p;tid++){
            size_t a=( tid   *vec_cnt)/omp_p;
            size_t b=((tid+1)*vec_cnt)/omp_p;
            for(size_t i=a;i<b;i++){
              const PERM_INT_T*  perm=(PERM_INT_T*)(precomp_data[0]+input_perm[(interac_indx+i)*4+0]);
              const     Real_t*  scal=(    Real_t*)(precomp_data[0]+input_perm[(interac_indx+i)*4+1]);
              const     Real_t* v_in =(    Real_t*)(  input_data[0]+input_perm[(interac_indx+i)*4+3]);
              Real_t*           v_out=(    Real_t*)(     buff_in   +input_perm[(interac_indx+i)*4+2]);
              for(size_t j=0;j<M_dim0;j++ ){
                v_out[j]=v_in[perm[j]]*scal[j];
              }
            }
          }
          size_t vec_cnt0=0;
          for(size_t j=interac_blk_dsp;j<interac_blk_dsp+interac_blk[k];){
            size_t vec_cnt1=0;
            size_t interac_mat0=interac_mat[j];
            for(;j<interac_blk_dsp+interac_blk[k] && interac_mat[j]==interac_mat0;j++) vec_cnt1+=interac_cnt[j];
            Matrix<Real_t> M(M_dim0, M_dim1, (Real_t*)(precomp_data[0]+interac_mat0), false);
#pragma omp parallel for
            for(int tid=0;tid<omp_p;tid++){
              size_t a=(dof*vec_cnt1*(tid  ))/omp_p;
              size_t b=(dof*vec_cnt1*(tid+1))/omp_p;
              Matrix<Real_t> Ms(b-a, M_dim0, (Real_t*)(buff_in +M_dim0*vec_cnt0*dof*sizeof(Real_t))+M_dim0*a, false);
              Matrix<Real_t> Mt(b-a, M_dim1, (Real_t*)(buff_out+M_dim1*vec_cnt0*dof*sizeof(Real_t))+M_dim1*a, false);
              Matrix<Real_t>::GEMM(Mt,Ms,M);
            }
            vec_cnt0+=vec_cnt1;
          }
#pragma omp parallel for
          for(int tid=0;tid<omp_p;tid++){
            size_t a=( tid   *vec_cnt)/omp_p;
            size_t b=((tid+1)*vec_cnt)/omp_p;
            if(tid>      0 && a<vec_cnt){
              size_t out_ptr=output_perm[(interac_indx+a)*4+3];
              if(tid>      0) while(a<vec_cnt && out_ptr==output_perm[(interac_indx+a)*4+3]) a++;
            }
            if(tid<omp_p-1 && b<vec_cnt){
              size_t out_ptr=output_perm[(interac_indx+b)*4+3];
              if(tid<omp_p-1) while(b<vec_cnt && out_ptr==output_perm[(interac_indx+b)*4+3]) b++;
            }
            for(size_t i=a;i<b;i++){ // Compute permutations.
              const PERM_INT_T*  perm=(PERM_INT_T*)(precomp_data[0]+output_perm[(interac_indx+i)*4+0]);
              const     Real_t*  scal=(    Real_t*)(precomp_data[0]+output_perm[(interac_indx+i)*4+1]);
              const     Real_t* v_in =(    Real_t*)(    buff_out   +output_perm[(interac_indx+i)*4+2]);
              Real_t*           v_out=(    Real_t*)( output_data[0]+output_perm[(interac_indx+i)*4+3]);
              for(size_t j=0;j<M_dim1;j++ ){
                v_out[j]+=v_in[perm[j]]*scal[j];
              }
            }
          }
          interac_indx+=vec_cnt;
          interac_blk_dsp+=interac_blk[k];
        }
      }
    }
    Profile::Toc();
  }

  void PtSetup(SetupData<Real_t,FMMNode_t>& setup_data, ptSetupData* data_){
    ptSetupData& data=*(ptSetupData*)data_;
    if(data.interac_data.interac_cnt.Dim()){
      InteracData& intdata=data.interac_data;
      Vector<size_t>  cnt;
      Vector<size_t>& dsp=intdata.interac_cst;
      cnt.ReInit(intdata.interac_cnt.Dim());
      dsp.ReInit(intdata.interac_dsp.Dim());
#pragma omp parallel for
      for(size_t trg=0;trg<cnt.Dim();trg++){
        size_t trg_cnt=data.trg_coord.cnt[trg];
        cnt[trg]=0;
        for(size_t i=0;i<intdata.interac_cnt[trg];i++){
          size_t int_id=intdata.interac_dsp[trg]+i;
          size_t src=intdata.in_node[int_id];
          size_t src_cnt=data.src_coord.cnt[src];
          size_t srf_cnt=data.srf_coord.cnt[src];
          cnt[trg]+=(src_cnt+srf_cnt)*trg_cnt;
        }
      }
      dsp[0]=cnt[0];
      omp_par::scan(&cnt[0],&dsp[0],dsp.Dim());
    }
    {
      struct PackedSetupData{
        size_t size;
        int level;
        const Kernel<Real_t>* kernel;
        Matrix<Real_t>* src_coord;
        Matrix<Real_t>* src_value;
        Matrix<Real_t>* srf_coord;
        Matrix<Real_t>* srf_value;
        Matrix<Real_t>* trg_coord;
        Matrix<Real_t>* trg_value;
        size_t src_coord_cnt_size; size_t src_coord_cnt_offset;
        size_t src_coord_dsp_size; size_t src_coord_dsp_offset;
        size_t src_value_cnt_size; size_t src_value_cnt_offset;
        size_t src_value_dsp_size; size_t src_value_dsp_offset;
        size_t srf_coord_cnt_size; size_t srf_coord_cnt_offset;
        size_t srf_coord_dsp_size; size_t srf_coord_dsp_offset;
        size_t srf_value_cnt_size; size_t srf_value_cnt_offset;
        size_t srf_value_dsp_size; size_t srf_value_dsp_offset;
        size_t trg_coord_cnt_size; size_t trg_coord_cnt_offset;
        size_t trg_coord_dsp_size; size_t trg_coord_dsp_offset;
        size_t trg_value_cnt_size; size_t trg_value_cnt_offset;
        size_t trg_value_dsp_size; size_t trg_value_dsp_offset;
        size_t          in_node_size; size_t           in_node_offset;
        size_t         scal_idx_size; size_t          scal_idx_offset;
        size_t      coord_shift_size; size_t       coord_shift_offset;
        size_t      interac_cnt_size; size_t       interac_cnt_offset;
        size_t      interac_dsp_size; size_t       interac_dsp_offset;
        size_t      interac_cst_size; size_t       interac_cst_offset;
        size_t scal_dim[4*MAX_DEPTH]; size_t scal_offset[4*MAX_DEPTH];
        size_t            Mdim[4][2]; size_t              M_offset[4];
      };
      PackedSetupData pkd_data;
      {
        size_t offset=mem::align_ptr(sizeof(PackedSetupData));
        pkd_data. level=data. level;
        pkd_data.kernel=data.kernel;
        pkd_data.src_coord=data.src_coord.ptr;
        pkd_data.src_value=data.src_value.ptr;
        pkd_data.srf_coord=data.srf_coord.ptr;
        pkd_data.srf_value=data.srf_value.ptr;
        pkd_data.trg_coord=data.trg_coord.ptr;
        pkd_data.trg_value=data.trg_value.ptr;
        pkd_data.src_coord_cnt_offset=offset; pkd_data.src_coord_cnt_size=data.src_coord.cnt.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.src_coord_cnt_size);
        pkd_data.src_coord_dsp_offset=offset; pkd_data.src_coord_dsp_size=data.src_coord.dsp.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.src_coord_dsp_size);
        pkd_data.src_value_cnt_offset=offset; pkd_data.src_value_cnt_size=data.src_value.cnt.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.src_value_cnt_size);
        pkd_data.src_value_dsp_offset=offset; pkd_data.src_value_dsp_size=data.src_value.dsp.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.src_value_dsp_size);
        pkd_data.srf_coord_cnt_offset=offset; pkd_data.srf_coord_cnt_size=data.srf_coord.cnt.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.srf_coord_cnt_size);
        pkd_data.srf_coord_dsp_offset=offset; pkd_data.srf_coord_dsp_size=data.srf_coord.dsp.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.srf_coord_dsp_size);
        pkd_data.srf_value_cnt_offset=offset; pkd_data.srf_value_cnt_size=data.srf_value.cnt.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.srf_value_cnt_size);
        pkd_data.srf_value_dsp_offset=offset; pkd_data.srf_value_dsp_size=data.srf_value.dsp.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.srf_value_dsp_size);
        pkd_data.trg_coord_cnt_offset=offset; pkd_data.trg_coord_cnt_size=data.trg_coord.cnt.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.trg_coord_cnt_size);
        pkd_data.trg_coord_dsp_offset=offset; pkd_data.trg_coord_dsp_size=data.trg_coord.dsp.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.trg_coord_dsp_size);
        pkd_data.trg_value_cnt_offset=offset; pkd_data.trg_value_cnt_size=data.trg_value.cnt.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.trg_value_cnt_size);
        pkd_data.trg_value_dsp_offset=offset; pkd_data.trg_value_dsp_size=data.trg_value.dsp.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.trg_value_dsp_size);
        InteracData& intdata=data.interac_data;
        pkd_data.    in_node_offset=offset; pkd_data.    in_node_size=intdata.    in_node.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.    in_node_size);
        pkd_data.   scal_idx_offset=offset; pkd_data.   scal_idx_size=intdata.   scal_idx.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.   scal_idx_size);
        pkd_data.coord_shift_offset=offset; pkd_data.coord_shift_size=intdata.coord_shift.Dim(); offset+=mem::align_ptr(sizeof(Real_t)*pkd_data.coord_shift_size);
        pkd_data.interac_cnt_offset=offset; pkd_data.interac_cnt_size=intdata.interac_cnt.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.interac_cnt_size);
        pkd_data.interac_dsp_offset=offset; pkd_data.interac_dsp_size=intdata.interac_dsp.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.interac_dsp_size);
        pkd_data.interac_cst_offset=offset; pkd_data.interac_cst_size=intdata.interac_cst.Dim(); offset+=mem::align_ptr(sizeof(size_t)*pkd_data.interac_cst_size);
        for(size_t i=0;i<4*MAX_DEPTH;i++){
          pkd_data.scal_offset[i]=offset; pkd_data.scal_dim[i]=intdata.scal[i].Dim(); offset+=mem::align_ptr(sizeof(Real_t)*pkd_data.scal_dim[i]);
        }
        for(size_t i=0;i<4;i++){
          size_t& Mdim0=pkd_data.Mdim[i][0];
          size_t& Mdim1=pkd_data.Mdim[i][1];
          pkd_data.M_offset[i]=offset; Mdim0=intdata.M[i].Dim(0); Mdim1=intdata.M[i].Dim(1); offset+=mem::align_ptr(sizeof(Real_t)*Mdim0*Mdim1);
        }
        pkd_data.size=offset;
      }
      {
        Matrix<char>& buff=setup_data.interac_data;
        if(pkd_data.size>buff.Dim(0)*buff.Dim(1)){
          buff.ReInit(1,pkd_data.size);
        }
        ((PackedSetupData*)buff[0])[0]=pkd_data;
        if(pkd_data.src_coord_cnt_size) memcpy(&buff[0][pkd_data.src_coord_cnt_offset], &data.src_coord.cnt[0], pkd_data.src_coord_cnt_size*sizeof(size_t));
        if(pkd_data.src_coord_dsp_size) memcpy(&buff[0][pkd_data.src_coord_dsp_offset], &data.src_coord.dsp[0], pkd_data.src_coord_dsp_size*sizeof(size_t));
        if(pkd_data.src_value_cnt_size) memcpy(&buff[0][pkd_data.src_value_cnt_offset], &data.src_value.cnt[0], pkd_data.src_value_cnt_size*sizeof(size_t));
        if(pkd_data.src_value_dsp_size) memcpy(&buff[0][pkd_data.src_value_dsp_offset], &data.src_value.dsp[0], pkd_data.src_value_dsp_size*sizeof(size_t));
        if(pkd_data.srf_coord_cnt_size) memcpy(&buff[0][pkd_data.srf_coord_cnt_offset], &data.srf_coord.cnt[0], pkd_data.srf_coord_cnt_size*sizeof(size_t));
        if(pkd_data.srf_coord_dsp_size) memcpy(&buff[0][pkd_data.srf_coord_dsp_offset], &data.srf_coord.dsp[0], pkd_data.srf_coord_dsp_size*sizeof(size_t));
        if(pkd_data.srf_value_cnt_size) memcpy(&buff[0][pkd_data.srf_value_cnt_offset], &data.srf_value.cnt[0], pkd_data.srf_value_cnt_size*sizeof(size_t));
        if(pkd_data.srf_value_dsp_size) memcpy(&buff[0][pkd_data.srf_value_dsp_offset], &data.srf_value.dsp[0], pkd_data.srf_value_dsp_size*sizeof(size_t));
        if(pkd_data.trg_coord_cnt_size) memcpy(&buff[0][pkd_data.trg_coord_cnt_offset], &data.trg_coord.cnt[0], pkd_data.trg_coord_cnt_size*sizeof(size_t));
        if(pkd_data.trg_coord_dsp_size) memcpy(&buff[0][pkd_data.trg_coord_dsp_offset], &data.trg_coord.dsp[0], pkd_data.trg_coord_dsp_size*sizeof(size_t));
        if(pkd_data.trg_value_cnt_size) memcpy(&buff[0][pkd_data.trg_value_cnt_offset], &data.trg_value.cnt[0], pkd_data.trg_value_cnt_size*sizeof(size_t));
        if(pkd_data.trg_value_dsp_size) memcpy(&buff[0][pkd_data.trg_value_dsp_offset], &data.trg_value.dsp[0], pkd_data.trg_value_dsp_size*sizeof(size_t));
        InteracData& intdata=data.interac_data;
        if(pkd_data.    in_node_size) memcpy(&buff[0][pkd_data.    in_node_offset], &intdata.    in_node[0], pkd_data.    in_node_size*sizeof(size_t));
        if(pkd_data.   scal_idx_size) memcpy(&buff[0][pkd_data.   scal_idx_offset], &intdata.   scal_idx[0], pkd_data.   scal_idx_size*sizeof(size_t));
        if(pkd_data.coord_shift_size) memcpy(&buff[0][pkd_data.coord_shift_offset], &intdata.coord_shift[0], pkd_data.coord_shift_size*sizeof(Real_t));
        if(pkd_data.interac_cnt_size) memcpy(&buff[0][pkd_data.interac_cnt_offset], &intdata.interac_cnt[0], pkd_data.interac_cnt_size*sizeof(size_t));
        if(pkd_data.interac_dsp_size) memcpy(&buff[0][pkd_data.interac_dsp_offset], &intdata.interac_dsp[0], pkd_data.interac_dsp_size*sizeof(size_t));
        if(pkd_data.interac_cst_size) memcpy(&buff[0][pkd_data.interac_cst_offset], &intdata.interac_cst[0], pkd_data.interac_cst_size*sizeof(size_t));
        for(size_t i=0;i<4*MAX_DEPTH;i++){
          if(intdata.scal[i].Dim()) memcpy(&buff[0][pkd_data.scal_offset[i]], &intdata.scal[i][0], intdata.scal[i].Dim()*sizeof(Real_t));
        }
        for(size_t i=0;i<4;i++){
          if(intdata.M[i].Dim(0)*intdata.M[i].Dim(1)) memcpy(&buff[0][pkd_data.M_offset[i]], &intdata.M[i][0][0], intdata.M[i].Dim(0)*intdata.M[i].Dim(1)*sizeof(Real_t));
        }
      }
    }
    {
      size_t n=setup_data.output_data->Dim(0)*setup_data.output_data->Dim(1)*sizeof(Real_t);
      if(this->dev_buffer.Dim()<n) this->dev_buffer.ReInit(n);
    }
  }

  void Source2UpSetup(SetupData<Real_t,FMMNode_t>& setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& buff, std::vector<Vector<FMMNode_t*> >& n_list, int level) {
    if(!this->MultipoleOrder()) return;
    {
      setup_data. level=level;
      setup_data.kernel=kernel->k_s2m;
      setup_data. input_data=&buff[4];
      setup_data.output_data=&buff[0];
      setup_data. coord_data=&buff[6];
      Vector<FMMNode_t*>& nodes_in =n_list[4];
      Vector<FMMNode_t*>& nodes_out=n_list[0];
      setup_data.nodes_in .clear();
      setup_data.nodes_out.clear();
      for(size_t i=0;i<nodes_in .Dim();i++)
        if((nodes_in [i]->depth==level || level==-1)
  	 && (nodes_in [i]->src_coord.Dim() || nodes_in [i]->surf_coord.Dim())
  	 && nodes_in [i]->IsLeaf() && !nodes_in [i]->IsGhost()) setup_data.nodes_in .push_back(nodes_in [i]);
      for(size_t i=0;i<nodes_out.Dim();i++)
        if((nodes_out[i]->depth==level || level==-1)
  	 && (nodes_out[i]->src_coord.Dim() || nodes_out[i]->surf_coord.Dim())
  	 && nodes_out[i]->IsLeaf() && !nodes_out[i]->IsGhost()) setup_data.nodes_out.push_back(nodes_out[i]);
    }
    ptSetupData data;
    data. level=setup_data. level;
    data.kernel=setup_data.kernel;
    std::vector<FMMNode_t*>& nodes_in =setup_data.nodes_in ;
    std::vector<FMMNode_t*>& nodes_out=setup_data.nodes_out;
    {
      std::vector<FMMNode_t*>& nodes=nodes_in;
      PackedData& coord=data.src_coord;
      PackedData& value=data.src_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data. input_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        nodes[i]->node_id=i;
        Vector<Real_t>& coord_vec=nodes[i]->src_coord;
        Vector<Real_t>& value_vec=nodes[i]->src_value;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      std::vector<FMMNode_t*>& nodes=nodes_in;
      PackedData& coord=data.srf_coord;
      PackedData& value=data.srf_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data. input_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        Vector<Real_t>& coord_vec=nodes[i]->surf_coord;
        Vector<Real_t>& value_vec=nodes[i]->surf_value;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      std::vector<FMMNode_t*>& nodes=nodes_out;
      PackedData& coord=data.trg_coord;
      PackedData& value=data.trg_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data.output_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        Vector<Real_t>& coord_vec=tree->upwd_check_surf[nodes[i]->depth];
        Vector<Real_t>& value_vec=(nodes[i]->FMMData())->upward_equiv;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      int omp_p=omp_get_max_threads();
      std::vector<std::vector<size_t> > in_node_(omp_p);
      std::vector<std::vector<size_t> > scal_idx_(omp_p);
      std::vector<std::vector<Real_t> > coord_shift_(omp_p);
      std::vector<std::vector<size_t> > interac_cnt_(omp_p);
      if(this->ScaleInvar()){
        const Kernel<Real_t>* ker=kernel->k_m2m;
        for(size_t l=0;l<MAX_DEPTH;l++){
          Vector<Real_t>& scal=data.interac_data.scal[l*4+2];
          Vector<Real_t>& scal_exp=ker->trg_scal;
          scal.ReInit(scal_exp.Dim());
          for(size_t i=0;i<scal.Dim();i++){
            scal[i]=pvfmm::pow<Real_t>(2.0,-scal_exp[i]*l);
          }
        }
        for(size_t l=0;l<MAX_DEPTH;l++){
          Vector<Real_t>& scal=data.interac_data.scal[l*4+3];
          Vector<Real_t>& scal_exp=ker->src_scal;
          scal.ReInit(scal_exp.Dim());
          for(size_t i=0;i<scal.Dim();i++){
            scal[i]=pvfmm::pow<Real_t>(2.0,-scal_exp[i]*l);
          }
        }
      }
#pragma omp parallel for
      for(size_t tid=0;tid<omp_p;tid++){
        std::vector<size_t>& in_node    =in_node_[tid]    ;
        std::vector<size_t>& scal_idx   =scal_idx_[tid]   ;
        std::vector<Real_t>& coord_shift=coord_shift_[tid];
        std::vector<size_t>& interac_cnt=interac_cnt_[tid];
        size_t a=(nodes_out.size()*(tid+0))/omp_p;
        size_t b=(nodes_out.size()*(tid+1))/omp_p;
        for(size_t i=a;i<b;i++){
          FMMNode_t* tnode=nodes_out[i];
          Real_t s=pvfmm::pow<Real_t>(0.5,tnode->depth);
          size_t interac_cnt_=0;
          {
            Mat_Type type=S2U_Type;
            Vector<FMMNode_t*>& intlst=tnode->interac_list[type];
            for(size_t j=0;j<intlst.Dim();j++) if(intlst[j]){
              FMMNode_t* snode=intlst[j];
              size_t snode_id=snode->node_id;
              if(snode_id>=nodes_in.size() || nodes_in[snode_id]!=snode) continue;
              in_node.push_back(snode_id);
              scal_idx.push_back(snode->depth);
              {
                const int* rel_coord=interac_list.RelativeCoord(type,j);
                const Real_t* scoord=snode->Coord();
                const Real_t* tcoord=tnode->Coord();
                Real_t shift[COORD_DIM];
                shift[0]=rel_coord[0]*0.5*s-(scoord[0]+0.5*s)+(0+0.5*s);
                shift[1]=rel_coord[1]*0.5*s-(scoord[1]+0.5*s)+(0+0.5*s);
                shift[2]=rel_coord[2]*0.5*s-(scoord[2]+0.5*s)+(0+0.5*s);
                coord_shift.push_back(shift[0]);
                coord_shift.push_back(shift[1]);
                coord_shift.push_back(shift[2]);
              }
              interac_cnt_++;
            }
          }
          interac_cnt.push_back(interac_cnt_);
        }
      }
      {
        InteracData& interac_data=data.interac_data;
	CopyVec(in_node_,interac_data.in_node);
	CopyVec(scal_idx_,interac_data.scal_idx);
	CopyVec(coord_shift_,interac_data.coord_shift);
	CopyVec(interac_cnt_,interac_data.interac_cnt);
        {
          pvfmm::Vector<size_t>& cnt=interac_data.interac_cnt;
          pvfmm::Vector<size_t>& dsp=interac_data.interac_dsp;
          dsp.ReInit(cnt.Dim()); if(dsp.Dim()) dsp[0]=0;
          omp_par::scan(&cnt[0],&dsp[0],dsp.Dim());
        }
      }
      {
        InteracData& interac_data=data.interac_data;
        pvfmm::Vector<size_t>& cnt=interac_data.interac_cnt;
        pvfmm::Vector<size_t>& dsp=interac_data.interac_dsp;
        if(cnt.Dim() && cnt[cnt.Dim()-1]+dsp[dsp.Dim()-1]){
          data.interac_data.M[2]=this->mat->Mat(level, UC2UE0_Type, 0);
          data.interac_data.M[3]=this->mat->Mat(level, UC2UE1_Type, 0);
        }else{
          data.interac_data.M[2].ReInit(0,0);
          data.interac_data.M[3].ReInit(0,0);
        }
      }
    }
    PtSetup(setup_data, &data);
  }

  void Source2Up(SetupData<Real_t,FMMNode_t>&  setup_data) {
    if(!this->MultipoleOrder()) return;
    this->EvalListPts(setup_data);
  }
  
  void Up2UpSetup(SetupData<Real_t,FMMNode_t>& setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& buff, std::vector<Vector<FMMNode_t*> >& n_list, int level){
    if(!this->MultipoleOrder()) return;
    {
      setup_data.level=level;
      setup_data.kernel=kernel->k_m2m;
      setup_data.interac_type.resize(1);
      setup_data.interac_type[0]=U2U_Type;
      setup_data. input_data=&buff[0];
      setup_data.output_data=&buff[0];
      Vector<FMMNode_t*>& nodes_in =n_list[0];
      Vector<FMMNode_t*>& nodes_out=n_list[0];
      setup_data.nodes_in .clear();
      setup_data.nodes_out.clear();
      for(size_t i=0;i<nodes_in .Dim();i++) if((nodes_in [i]->depth==level+1) && nodes_in [i]->pt_cnt[0]) setup_data.nodes_in .push_back(nodes_in [i]);
      for(size_t i=0;i<nodes_out.Dim();i++) if((nodes_out[i]->depth==level  ) && nodes_out[i]->pt_cnt[0]) setup_data.nodes_out.push_back(nodes_out[i]);
    }
    std::vector<FMMNode_t*>& nodes_in =setup_data.nodes_in ;
    std::vector<FMMNode_t*>& nodes_out=setup_data.nodes_out;
    std::vector<Vector<Real_t>*>&  input_vector=setup_data. input_vector;  input_vector.clear();
    std::vector<Vector<Real_t>*>& output_vector=setup_data.output_vector; output_vector.clear();
    for(size_t i=0;i<nodes_in .size();i++)  input_vector.push_back(&(nodes_in [i]->FMMData())->upward_equiv);
    for(size_t i=0;i<nodes_out.size();i++) output_vector.push_back(&(nodes_out[i]->FMMData())->upward_equiv);
    SetupInterac(setup_data);
  }
  
  void Up2Up(SetupData<Real_t,FMMNode_t>& setup_data){
    if(!this->MultipoleOrder()) return;
    EvalList(setup_data);
  }

  void PeriodicBC(FMMNode_t* node){
    if(!this->ScaleInvar() || this->MultipoleOrder()==0) return;
    Matrix<Real_t>& M = Precomp(0, BC_Type, 0);
    assert(node->FMMData()->upward_equiv.Dim()>0);
    int dof=1;
    Vector<Real_t>& upward_equiv=node->FMMData()->upward_equiv;
    Vector<Real_t>& dnward_equiv=node->FMMData()->dnward_equiv;
    assert(upward_equiv.Dim()==M.Dim(0)*dof);
    assert(dnward_equiv.Dim()==M.Dim(1)*dof);
    Matrix<Real_t> d_equiv(dof,M.Dim(1),&dnward_equiv[0],false);
    Matrix<Real_t> u_equiv(dof,M.Dim(0),&upward_equiv[0],false);
    Matrix<Real_t>::GEMM(d_equiv,u_equiv,M);
  }
    
  void UpwardPass() {
    int max_depth=0;
    {
      int max_depth_loc=0;
      std::vector<FMMNode_t*>& nodes=this->GetNodeList();
      for(size_t i=0;i<nodes.size();i++){
        FMMNode_t* n=nodes[i];
        if(n->depth>max_depth_loc) max_depth_loc=n->depth;
      }
      max_depth = max_depth_loc;
    }
    Profile::Tic("S2U",false,5);
    for(int i=0; i<=(this->ScaleInvar()?0:max_depth); i++){
      if(!this->ScaleInvar()) this->SetupPrecomp(setup_data[i+MAX_DEPTH*6]);
      this->Source2Up(setup_data[i+MAX_DEPTH*6]);
    }
    Profile::Toc();
    Profile::Tic("U2U",false,5);
    for(int i=max_depth-1; i>=0; i--){
      if(!this->ScaleInvar()) this->SetupPrecomp(setup_data[i+MAX_DEPTH*7]);
      this->Up2Up(setup_data[i+MAX_DEPTH*7]);
    }
    Profile::Toc();
  }
  
  void BuildInteracLists() {
    std::vector<FMMNode_t*> n_list_src;
    std::vector<FMMNode_t*> n_list_trg;
    {
      std::vector<FMMNode_t*>& nodes=this->GetNodeList();
      for(size_t i=0;i<nodes.size();i++){
        if(!nodes[i]->IsGhost() && nodes[i]->pt_cnt[0]){
          n_list_src.push_back(nodes[i]);
        }
        if(!nodes[i]->IsGhost() && nodes[i]->pt_cnt[1]){
          n_list_trg.push_back(nodes[i]);
        }
      }
    }
    size_t node_cnt=std::max(n_list_src.size(),n_list_trg.size());
    std::vector<Mat_Type> type_lst;
    std::vector<std::vector<FMMNode_t*>*> type_node_lst;
    type_lst.push_back(S2U_Type); type_node_lst.push_back(&n_list_src);
    type_lst.push_back(U2U_Type); type_node_lst.push_back(&n_list_src);
    type_lst.push_back(D2D_Type); type_node_lst.push_back(&n_list_trg);
    type_lst.push_back(D2T_Type); type_node_lst.push_back(&n_list_trg);
    type_lst.push_back(U0_Type ); type_node_lst.push_back(&n_list_trg);
    type_lst.push_back(U1_Type ); type_node_lst.push_back(&n_list_trg);
    type_lst.push_back(U2_Type ); type_node_lst.push_back(&n_list_trg);
    type_lst.push_back(W_Type  ); type_node_lst.push_back(&n_list_trg);
    type_lst.push_back(X_Type  ); type_node_lst.push_back(&n_list_trg);
    type_lst.push_back(V1_Type ); type_node_lst.push_back(&n_list_trg);
    std::vector<size_t> interac_cnt(type_lst.size());
    std::vector<size_t> interac_dsp(type_lst.size(),0);
    for(size_t i=0;i<type_lst.size();i++){
      interac_cnt[i]=interac_list.ListCount(type_lst[i]);
    }
    omp_par::scan(&interac_cnt[0],&interac_dsp[0],type_lst.size());
    node_interac_lst.ReInit(node_cnt,interac_cnt.back()+interac_dsp.back());
    int omp_p=omp_get_max_threads();
#pragma omp parallel for
    for(int j=0;j<omp_p;j++){
      for(size_t k=0;k<type_lst.size();k++){
        std::vector<FMMNode_t*>& n_list=*type_node_lst[k];
        size_t a=(n_list.size()*(j  ))/omp_p;
        size_t b=(n_list.size()*(j+1))/omp_p;
        for(size_t i=a;i<b;i++){
          FMMNode_t* n=n_list[i];
          n->interac_list[type_lst[k]].ReInit(interac_cnt[k],&node_interac_lst[i][interac_dsp[k]],false);
          interac_list.BuildList(n,type_lst[k]);
        }
      }
    }
  }

  void EvalListPts(SetupData<Real_t,FMMNode_t>& setup_data) {
    if(setup_data.kernel->ker_dim[0]*setup_data.kernel->ker_dim[1]==0) return;
    if(setup_data.interac_data.Dim(0)==0 || setup_data.interac_data.Dim(1)==0){
      return;
    }
    bool have_gpu=false;
    Profile::Tic("Host2Device",false,25);
    typename Vector<char>::Device      dev_buff;
    typename Matrix<char>::Device  interac_data;
    typename Matrix<Real_t>::Device  coord_data;
    typename Matrix<Real_t>::Device  input_data;
    typename Matrix<Real_t>::Device output_data;
    size_t ptr_single_layer_kernel=(size_t)NULL;
    size_t ptr_double_layer_kernel=(size_t)NULL;
    dev_buff    =       this-> dev_buffer;
    interac_data= setup_data.interac_data;
    if(setup_data.  coord_data!=NULL) coord_data  =*setup_data.  coord_data;
    if(setup_data.  input_data!=NULL) input_data  =*setup_data.  input_data;
    if(setup_data. output_data!=NULL) output_data =*setup_data. output_data;
    ptr_single_layer_kernel=(size_t)setup_data.kernel->ker_poten;
    ptr_double_layer_kernel=(size_t)setup_data.kernel->dbl_layer_poten;
    Profile::Toc();
    Profile::Tic("DeviceComp",false,20);
    int lock_idx=-1;
    int wait_lock_idx=-1;
    {
      ptSetupData data;
      {
        struct PackedSetupData{
          size_t size;
          int level;
          const Kernel<Real_t>* kernel;
          Matrix<Real_t>* src_coord;
          Matrix<Real_t>* src_value;
          Matrix<Real_t>* srf_coord;
          Matrix<Real_t>* srf_value;
          Matrix<Real_t>* trg_coord;
          Matrix<Real_t>* trg_value;
          size_t src_coord_cnt_size; size_t src_coord_cnt_offset;
          size_t src_coord_dsp_size; size_t src_coord_dsp_offset;
          size_t src_value_cnt_size; size_t src_value_cnt_offset;
          size_t src_value_dsp_size; size_t src_value_dsp_offset;
          size_t srf_coord_cnt_size; size_t srf_coord_cnt_offset;
          size_t srf_coord_dsp_size; size_t srf_coord_dsp_offset;
          size_t srf_value_cnt_size; size_t srf_value_cnt_offset;
          size_t srf_value_dsp_size; size_t srf_value_dsp_offset;
          size_t trg_coord_cnt_size; size_t trg_coord_cnt_offset;
          size_t trg_coord_dsp_size; size_t trg_coord_dsp_offset;
          size_t trg_value_cnt_size; size_t trg_value_cnt_offset;
          size_t trg_value_dsp_size; size_t trg_value_dsp_offset;
          size_t          in_node_size; size_t           in_node_offset;
          size_t         scal_idx_size; size_t          scal_idx_offset;
          size_t      coord_shift_size; size_t       coord_shift_offset;
          size_t      interac_cnt_size; size_t       interac_cnt_offset;
          size_t      interac_dsp_size; size_t       interac_dsp_offset;
          size_t      interac_cst_size; size_t       interac_cst_offset;
          size_t scal_dim[4*MAX_DEPTH]; size_t scal_offset[4*MAX_DEPTH];
          size_t            Mdim[4][2]; size_t              M_offset[4];
        };
        typename Matrix<char>::Device& setupdata=interac_data;
        PackedSetupData& pkd_data=*((PackedSetupData*)setupdata[0]);
        data. level=pkd_data. level;
        data.kernel=pkd_data.kernel;
        data.src_coord.ptr=pkd_data.src_coord;
        data.src_value.ptr=pkd_data.src_value;
        data.srf_coord.ptr=pkd_data.srf_coord;
        data.srf_value.ptr=pkd_data.srf_value;
        data.trg_coord.ptr=pkd_data.trg_coord;
        data.trg_value.ptr=pkd_data.trg_value;
        data.src_coord.cnt.ReInit(pkd_data.src_coord_cnt_size, (size_t*)&setupdata[0][pkd_data.src_coord_cnt_offset], false);
        data.src_coord.dsp.ReInit(pkd_data.src_coord_dsp_size, (size_t*)&setupdata[0][pkd_data.src_coord_dsp_offset], false);
        data.src_value.cnt.ReInit(pkd_data.src_value_cnt_size, (size_t*)&setupdata[0][pkd_data.src_value_cnt_offset], false);
        data.src_value.dsp.ReInit(pkd_data.src_value_dsp_size, (size_t*)&setupdata[0][pkd_data.src_value_dsp_offset], false);
        data.srf_coord.cnt.ReInit(pkd_data.srf_coord_cnt_size, (size_t*)&setupdata[0][pkd_data.srf_coord_cnt_offset], false);
        data.srf_coord.dsp.ReInit(pkd_data.srf_coord_dsp_size, (size_t*)&setupdata[0][pkd_data.srf_coord_dsp_offset], false);
        data.srf_value.cnt.ReInit(pkd_data.srf_value_cnt_size, (size_t*)&setupdata[0][pkd_data.srf_value_cnt_offset], false);
        data.srf_value.dsp.ReInit(pkd_data.srf_value_dsp_size, (size_t*)&setupdata[0][pkd_data.srf_value_dsp_offset], false);
        data.trg_coord.cnt.ReInit(pkd_data.trg_coord_cnt_size, (size_t*)&setupdata[0][pkd_data.trg_coord_cnt_offset], false);
        data.trg_coord.dsp.ReInit(pkd_data.trg_coord_dsp_size, (size_t*)&setupdata[0][pkd_data.trg_coord_dsp_offset], false);
        data.trg_value.cnt.ReInit(pkd_data.trg_value_cnt_size, (size_t*)&setupdata[0][pkd_data.trg_value_cnt_offset], false);
        data.trg_value.dsp.ReInit(pkd_data.trg_value_dsp_size, (size_t*)&setupdata[0][pkd_data.trg_value_dsp_offset], false);
        InteracData& intdata=data.interac_data;
        intdata.    in_node.ReInit(pkd_data.    in_node_size, (size_t*)&setupdata[0][pkd_data.    in_node_offset],false);
        intdata.   scal_idx.ReInit(pkd_data.   scal_idx_size, (size_t*)&setupdata[0][pkd_data.   scal_idx_offset],false);
        intdata.coord_shift.ReInit(pkd_data.coord_shift_size, (Real_t*)&setupdata[0][pkd_data.coord_shift_offset],false);
        intdata.interac_cnt.ReInit(pkd_data.interac_cnt_size, (size_t*)&setupdata[0][pkd_data.interac_cnt_offset],false);
        intdata.interac_dsp.ReInit(pkd_data.interac_dsp_size, (size_t*)&setupdata[0][pkd_data.interac_dsp_offset],false);
        intdata.interac_cst.ReInit(pkd_data.interac_cst_size, (size_t*)&setupdata[0][pkd_data.interac_cst_offset],false);
        for(size_t i=0;i<4*MAX_DEPTH;i++){
          intdata.scal[i].ReInit(pkd_data.scal_dim[i], (Real_t*)&setupdata[0][pkd_data.scal_offset[i]],false);
        }
        for(size_t i=0;i<4;i++){
          intdata.M[i].ReInit(pkd_data.Mdim[i][0], pkd_data.Mdim[i][1], (Real_t*)&setupdata[0][pkd_data.M_offset[i]],false);
        }
      }
      {
        InteracData& intdata=data.interac_data;
        typename Kernel<Real_t>::Ker_t single_layer_kernel=(typename Kernel<Real_t>::Ker_t)ptr_single_layer_kernel;
        typename Kernel<Real_t>::Ker_t double_layer_kernel=(typename Kernel<Real_t>::Ker_t)ptr_double_layer_kernel;
        int omp_p=omp_get_max_threads();
#pragma omp parallel for
        for(size_t tid=0;tid<omp_p;tid++){
          Matrix<Real_t> src_coord, src_value;
          Matrix<Real_t> srf_coord, srf_value;
          Matrix<Real_t> trg_coord, trg_value;
          Vector<Real_t> buff;
          {
            size_t thread_buff_size=dev_buff.dim/sizeof(Real_t)/omp_p;
            buff.ReInit(thread_buff_size, (Real_t*)&dev_buff[tid*thread_buff_size*sizeof(Real_t)], false);
          }
          size_t vcnt=0;
          std::vector<Matrix<Real_t> > vbuff(6);
          {
            size_t vdim_=0, vdim[6];
            for(size_t indx=0;indx<6;indx++){
              vdim[indx]=0;
              switch(indx){
                case 0:
                  vdim[indx]=intdata.M[0].Dim(0); break;
                case 1:
                  assert(intdata.M[0].Dim(1)==intdata.M[1].Dim(0));
                  vdim[indx]=intdata.M[0].Dim(1); break;
                case 2:
                  vdim[indx]=intdata.M[1].Dim(1); break;
                case 3:
                  vdim[indx]=intdata.M[2].Dim(0); break;
                case 4:
                  assert(intdata.M[2].Dim(1)==intdata.M[3].Dim(0));
                  vdim[indx]=intdata.M[2].Dim(1); break;
                case 5:
                  vdim[indx]=intdata.M[3].Dim(1); break;
                default:
                  vdim[indx]=0; break;
              }
              vdim_+=vdim[indx];
            }
            if(vdim_){
              vcnt=buff.Dim()/vdim_/2;
              assert(vcnt>0);
            }
            for(size_t indx=0;indx<6;indx++){
              vbuff[indx].ReInit(vcnt,vdim[indx],&buff[0],false);
              buff.ReInit(buff.Dim()-vdim[indx]*vcnt, &buff[vdim[indx]*vcnt], false);
            }
          }
          size_t trg_a=0, trg_b=0;
          if(intdata.interac_cst.Dim()){
            Vector<size_t>& interac_cst=intdata.interac_cst;
            size_t cost=interac_cst[interac_cst.Dim()-1];
            trg_a=std::lower_bound(&interac_cst[0],&interac_cst[interac_cst.Dim()-1],(cost*(tid+0))/omp_p)-&interac_cst[0]+1;
            trg_b=std::lower_bound(&interac_cst[0],&interac_cst[interac_cst.Dim()-1],(cost*(tid+1))/omp_p)-&interac_cst[0]+1;
            if(tid==omp_p-1) trg_b=interac_cst.Dim();
            if(tid==0) trg_a=0;
          }
          for(size_t trg0=trg_a;trg0<trg_b;){
            size_t trg1_max=1;
            if(vcnt){
              size_t interac_cnt=intdata.interac_cnt[trg0];
              while(trg0+trg1_max<trg_b){
                interac_cnt+=intdata.interac_cnt[trg0+trg1_max];
                if(interac_cnt>vcnt){
                  interac_cnt-=intdata.interac_cnt[trg0+trg1_max];
                  break;
                }
                trg1_max++;
              }
              assert(interac_cnt<=vcnt);
              for(size_t k=0;k<6;k++){
                if(vbuff[k].Dim(0)*vbuff[k].Dim(1)){
                  vbuff[k].ReInit(interac_cnt,vbuff[k].Dim(1),vbuff[k][0],false);
                }
              }
            }else{
              trg1_max=trg_b-trg0;
            }
            if(intdata.M[0].Dim(0) && intdata.M[0].Dim(1) && intdata.M[1].Dim(0) && intdata.M[1].Dim(1)){
              size_t interac_idx=0;
              for(size_t trg1=0;trg1<trg1_max;trg1++){
                size_t trg=trg0+trg1;
                for(size_t i=0;i<intdata.interac_cnt[trg];i++){
                  size_t int_id=intdata.interac_dsp[trg]+i;
                  size_t src=intdata.in_node[int_id];
                  src_value.ReInit(1, data.src_value.cnt[src], &data.src_value.ptr[0][0][data.src_value.dsp[src]], false);
                  {
                    size_t vdim=vbuff[0].Dim(1);
                    assert(src_value.Dim(1)==vdim);
                    for(size_t j=0;j<vdim;j++) vbuff[0][interac_idx][j]=src_value[0][j];
                  }
                  size_t scal_idx=intdata.scal_idx[int_id];
                  {
                    Matrix<Real_t>& vec=vbuff[0];
                    Vector<Real_t>& scal=intdata.scal[scal_idx*4+0];
                    size_t scal_dim=scal.Dim();
                    if(scal_dim){
                      size_t vdim=vec.Dim(1);
                      for(size_t j=0;j<vdim;j+=scal_dim){
                        for(size_t k=0;k<scal_dim;k++){
                          vec[interac_idx][j+k]*=scal[k];
                        }
                      }
                    }
                  }
                  interac_idx++;
                }
              }
              Matrix<Real_t>::GEMM(vbuff[1],vbuff[0],intdata.M[0]);
              Matrix<Real_t>::GEMM(vbuff[2],vbuff[1],intdata.M[1]);
              interac_idx=0;
              for(size_t trg1=0;trg1<trg1_max;trg1++){
                size_t trg=trg0+trg1;
                for(size_t i=0;i<intdata.interac_cnt[trg];i++){
                  size_t int_id=intdata.interac_dsp[trg]+i;
                  size_t scal_idx=intdata.scal_idx[int_id];
                  {
                    Matrix<Real_t>& vec=vbuff[2];
                    Vector<Real_t>& scal=intdata.scal[scal_idx*4+1];
                    size_t scal_dim=scal.Dim();
                    if(scal_dim){
                      size_t vdim=vec.Dim(1);
                      for(size_t j=0;j<vdim;j+=scal_dim){
                        for(size_t k=0;k<scal_dim;k++){
                          vec[interac_idx][j+k]*=scal[k];
                        }
                      }
                    }
                  }
                  interac_idx++;
                }
              }
            }
            if(intdata.M[2].Dim(0) && intdata.M[2].Dim(1) && intdata.M[3].Dim(0) && intdata.M[3].Dim(1)){
              size_t vdim=vbuff[3].Dim(0)*vbuff[3].Dim(1);
              for(size_t i=0;i<vdim;i++) vbuff[3][0][i]=0;
            }
            {
              size_t interac_idx=0;
              for(size_t trg1=0;trg1<trg1_max;trg1++){
                size_t trg=trg0+trg1;
                trg_coord.ReInit(1, data.trg_coord.cnt[trg], &data.trg_coord.ptr[0][0][data.trg_coord.dsp[trg]], false);
                trg_value.ReInit(1, data.trg_value.cnt[trg], &data.trg_value.ptr[0][0][data.trg_value.dsp[trg]], false);
                for(size_t i=0;i<intdata.interac_cnt[trg];i++){
                  size_t int_id=intdata.interac_dsp[trg]+i;
                  size_t src=intdata.in_node[int_id];
                  src_coord.ReInit(1, data.src_coord.cnt[src], &data.src_coord.ptr[0][0][data.src_coord.dsp[src]], false);
                  src_value.ReInit(1, data.src_value.cnt[src], &data.src_value.ptr[0][0][data.src_value.dsp[src]], false);
                  srf_coord.ReInit(1, data.srf_coord.cnt[src], &data.srf_coord.ptr[0][0][data.srf_coord.dsp[src]], false);
                  srf_value.ReInit(1, data.srf_value.cnt[src], &data.srf_value.ptr[0][0][data.srf_value.dsp[src]], false);
                  Real_t* vbuff2_ptr=(vbuff[2].Dim(0)*vbuff[2].Dim(1)?vbuff[2][interac_idx]:src_value[0]);
                  Real_t* vbuff3_ptr=(vbuff[3].Dim(0)*vbuff[3].Dim(1)?vbuff[3][interac_idx]:trg_value[0]);
                  if(src_coord.Dim(1)){
                    {
                      Real_t* shift=&intdata.coord_shift[int_id*COORD_DIM];
                      if(shift[0]!=0 || shift[1]!=0 || shift[2]!=0){
                        size_t vdim=src_coord.Dim(1);
                        Vector<Real_t> new_coord(vdim, &buff[0], false);
                        assert(buff.Dim()>=vdim);
                        for(size_t j=0;j<vdim;j+=COORD_DIM){
                          for(size_t k=0;k<COORD_DIM;k++){
                            new_coord[j+k]=src_coord[0][j+k]+shift[k];
                          }
                        }
                        src_coord.ReInit(1, vdim, &new_coord[0], false);
                      }
                    }
                    assert(ptr_single_layer_kernel);
                    single_layer_kernel(src_coord[0], src_coord.Dim(1)/COORD_DIM, vbuff2_ptr, 1,
                                        trg_coord[0], trg_coord.Dim(1)/COORD_DIM, vbuff3_ptr, NULL);
                  }
                  if(srf_coord.Dim(1)){
                    {
                      Real_t* shift=&intdata.coord_shift[int_id*COORD_DIM];
                      if(shift[0]!=0 || shift[1]!=0 || shift[2]!=0){
                        size_t vdim=srf_coord.Dim(1);
                        Vector<Real_t> new_coord(vdim, &buff[0], false);
                        assert(buff.Dim()>=vdim);
                        for(size_t j=0;j<vdim;j+=COORD_DIM){
                          for(size_t k=0;k<COORD_DIM;k++){
                            new_coord[j+k]=srf_coord[0][j+k]+shift[k];
                          }
                        }
                        srf_coord.ReInit(1, vdim, &new_coord[0], false);
                      }
                    }
                    assert(ptr_double_layer_kernel);
                    double_layer_kernel(srf_coord[0], srf_coord.Dim(1)/COORD_DIM, srf_value[0], 1,
                                        trg_coord[0], trg_coord.Dim(1)/COORD_DIM, vbuff3_ptr, NULL);
                  }
                  interac_idx++;
                }
              }
            }
            if(intdata.M[2].Dim(0) && intdata.M[2].Dim(1) && intdata.M[3].Dim(0) && intdata.M[3].Dim(1)){
              size_t interac_idx=0;
              for(size_t trg1=0;trg1<trg1_max;trg1++){
                size_t trg=trg0+trg1;
                for(size_t i=0;i<intdata.interac_cnt[trg];i++){
                  size_t int_id=intdata.interac_dsp[trg]+i;
                  size_t scal_idx=intdata.scal_idx[int_id];
                  {
                    Matrix<Real_t>& vec=vbuff[3];
                    Vector<Real_t>& scal=intdata.scal[scal_idx*4+2];
                    size_t scal_dim=scal.Dim();
                    if(scal_dim){
                      size_t vdim=vec.Dim(1);
                      for(size_t j=0;j<vdim;j+=scal_dim){
                        for(size_t k=0;k<scal_dim;k++){
                          vec[interac_idx][j+k]*=scal[k];
                        }
                      }
                    }
                  }
                  interac_idx++;
                }
              }
              Matrix<Real_t>::GEMM(vbuff[4],vbuff[3],intdata.M[2]);
              Matrix<Real_t>::GEMM(vbuff[5],vbuff[4],intdata.M[3]);
              interac_idx=0;
              for(size_t trg1=0;trg1<trg1_max;trg1++){
                size_t trg=trg0+trg1;
                trg_value.ReInit(1, data.trg_value.cnt[trg], &data.trg_value.ptr[0][0][data.trg_value.dsp[trg]], false);
                for(size_t i=0;i<intdata.interac_cnt[trg];i++){
                  size_t int_id=intdata.interac_dsp[trg]+i;
                  size_t scal_idx=intdata.scal_idx[int_id];
                  {
                    Matrix<Real_t>& vec=vbuff[5];
                    Vector<Real_t>& scal=intdata.scal[scal_idx*4+3];
                    size_t scal_dim=scal.Dim();
                    if(scal_dim){
                      size_t vdim=vec.Dim(1);
                      for(size_t j=0;j<vdim;j+=scal_dim){
                        for(size_t k=0;k<scal_dim;k++){
                          vec[interac_idx][j+k]*=scal[k];
                        }
                      }
                    }
                  }
                  {
                    size_t vdim=vbuff[5].Dim(1);
                    assert(trg_value.Dim(1)==vdim);
                    for(size_t i=0;i<vdim;i++) trg_value[0][i]+=vbuff[5][interac_idx][i];
                  }
                  interac_idx++;
                }
              }
            }
            trg0+=trg1_max;
          }
        }
      }
    }
    Profile::Toc();
  }  

  void V_ListSetup(SetupData<Real_t,FMMNode_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& buff, std::vector<Vector<FMMNode_t*> >& n_list, int level){
    if(!this->MultipoleOrder()) return;
    if(level==0) return;
    {
      setup_data.level=level;
      setup_data.kernel=kernel->k_m2l;
      setup_data.interac_type.resize(1);
      setup_data.interac_type[0]=V1_Type;
      setup_data. input_data=&buff[0];
      setup_data.output_data=&buff[1];
      Vector<FMMNode_t*>& nodes_in =n_list[2];
      Vector<FMMNode_t*>& nodes_out=n_list[3];
      setup_data.nodes_in .clear();
      setup_data.nodes_out.clear();
      for(size_t i=0;i<nodes_in .Dim();i++) if((nodes_in [i]->depth==level-1 || level==-1) && nodes_in [i]->pt_cnt[0]) setup_data.nodes_in .push_back(nodes_in [i]);
      for(size_t i=0;i<nodes_out.Dim();i++) if((nodes_out[i]->depth==level-1 || level==-1) && nodes_out[i]->pt_cnt[1]) setup_data.nodes_out.push_back(nodes_out[i]);
    }
    std::vector<FMMNode_t*>& nodes_in =setup_data.nodes_in ;
    std::vector<FMMNode_t*>& nodes_out=setup_data.nodes_out;
    std::vector<Vector<Real_t>*>&  input_vector=setup_data. input_vector;  input_vector.clear();
    std::vector<Vector<Real_t>*>& output_vector=setup_data.output_vector; output_vector.clear();
    for(size_t i=0;i<nodes_in .size();i++)  input_vector.push_back(&(nodes_in[i]->Child(0)->FMMData())->upward_equiv);
    for(size_t i=0;i<nodes_out.size();i++) output_vector.push_back(&(nodes_out[i]->Child(0)->FMMData())->dnward_equiv);
    Real_t eps=1e-10;
    size_t n_in =nodes_in .size();
    size_t n_out=nodes_out.size();
    Profile::Tic("Interac-Data",true,25);
    Matrix<char>& interac_data=setup_data.interac_data;
    if(n_out>0 && n_in >0){
      size_t precomp_offset=0;
      Mat_Type& interac_type=setup_data.interac_type[0];
      size_t mat_cnt=this->interac_list.ListCount(interac_type);
      Matrix<size_t> precomp_data_offset;
      std::vector<size_t> interac_mat;
      std::vector<Real_t*> interac_mat_ptr;
      {
        for(size_t mat_id=0;mat_id<mat_cnt;mat_id++){
          Matrix<Real_t>& M = this->mat->Mat(level, interac_type, mat_id);
          interac_mat_ptr.push_back(&M[0][0]);
        }
      }
      size_t dof;
      size_t m=MultipoleOrder();
      size_t ker_dim0=setup_data.kernel->ker_dim[0];
      size_t ker_dim1=setup_data.kernel->ker_dim[1];
      size_t fftsize;
      {
        size_t n1=m*2;
        size_t n2=n1*n1;
        size_t n3_=n2*(n1/2+1);
        size_t chld_cnt=1UL<<COORD_DIM;
        fftsize=2*n3_*chld_cnt;
        dof=1;
      }
      int omp_p=omp_get_max_threads();
      size_t buff_size=DEVICE_BUFFER_SIZE*1024l*1024l;
      size_t n_blk0=2*fftsize*dof*(ker_dim0*n_in +ker_dim1*n_out)*sizeof(Real_t)/buff_size;
      if(n_blk0==0) n_blk0=1;
      std::vector<std::vector<size_t> >  fft_vec(n_blk0);
      std::vector<std::vector<size_t> > ifft_vec(n_blk0);
      std::vector<std::vector<Real_t> >  fft_scl(n_blk0);
      std::vector<std::vector<Real_t> > ifft_scl(n_blk0);
      std::vector<std::vector<size_t> > interac_vec(n_blk0);
      std::vector<std::vector<size_t> > interac_dsp(n_blk0);
      {
        Matrix<Real_t>&  input_data=*setup_data. input_data;
        Matrix<Real_t>& output_data=*setup_data.output_data;
        std::vector<std::vector<FMMNode_t*> > nodes_blk_in (n_blk0);
        std::vector<std::vector<FMMNode_t*> > nodes_blk_out(n_blk0);
        Vector<Real_t> src_scal=this->kernel->k_m2l->src_scal;
        Vector<Real_t> trg_scal=this->kernel->k_m2l->trg_scal;
  
        for(size_t i=0;i<n_in;i++) nodes_in[i]->node_id=i;
        for(size_t blk0=0;blk0<n_blk0;blk0++){
          size_t blk0_start=(n_out* blk0   )/n_blk0;
          size_t blk0_end  =(n_out*(blk0+1))/n_blk0;
          std::vector<FMMNode_t*>& nodes_in_ =nodes_blk_in [blk0];
          std::vector<FMMNode_t*>& nodes_out_=nodes_blk_out[blk0];
          {
            std::set<FMMNode_t*> nodes_in;
            for(size_t i=blk0_start;i<blk0_end;i++){
              nodes_out_.push_back(nodes_out[i]);
              Vector<FMMNode_t*>& lst=nodes_out[i]->interac_list[interac_type];
              for(size_t k=0;k<mat_cnt;k++) if(lst[k]!=NULL && lst[k]->pt_cnt[0]) nodes_in.insert(lst[k]);
            }
            for(typename std::set<FMMNode_t*>::iterator node=nodes_in.begin(); node != nodes_in.end(); node++){
              nodes_in_.push_back(*node);
            }
            size_t  input_dim=nodes_in_ .size()*ker_dim0*dof*fftsize;
            size_t output_dim=nodes_out_.size()*ker_dim1*dof*fftsize;
            size_t buffer_dim=2*(ker_dim0+ker_dim1)*dof*fftsize*omp_p;
            if(buff_size<(input_dim + output_dim + buffer_dim)*sizeof(Real_t))
              buff_size=(input_dim + output_dim + buffer_dim)*sizeof(Real_t);
          }
          {
            for(size_t i=0;i<nodes_in_ .size();i++) fft_vec[blk0].push_back((size_t)(& input_vector[nodes_in_[i]->node_id][0][0]- input_data[0]));
            for(size_t i=0;i<nodes_out_.size();i++)ifft_vec[blk0].push_back((size_t)(&output_vector[blk0_start   +     i ][0][0]-output_data[0]));
            size_t scal_dim0=src_scal.Dim();
            size_t scal_dim1=trg_scal.Dim();
            fft_scl [blk0].resize(nodes_in_ .size()*scal_dim0);
            ifft_scl[blk0].resize(nodes_out_.size()*scal_dim1);
            for(size_t i=0;i<nodes_in_ .size();i++){
              size_t depth=nodes_in_[i]->depth+1;
              for(size_t j=0;j<scal_dim0;j++){
                fft_scl[blk0][i*scal_dim0+j]=pvfmm::pow<Real_t>(2.0, src_scal[j]*depth);
              }
            }
            for(size_t i=0;i<nodes_out_.size();i++){
              size_t depth=nodes_out_[i]->depth+1;
              for(size_t j=0;j<scal_dim1;j++){
                ifft_scl[blk0][i*scal_dim1+j]=pvfmm::pow<Real_t>(2.0, trg_scal[j]*depth);
              }
            }
          }
        }
        for(size_t blk0=0;blk0<n_blk0;blk0++){
          std::vector<FMMNode_t*>& nodes_in_ =nodes_blk_in [blk0];
          std::vector<FMMNode_t*>& nodes_out_=nodes_blk_out[blk0];
          for(size_t i=0;i<nodes_in_.size();i++) nodes_in_[i]->node_id=i;
          {
            size_t n_blk1=nodes_out_.size()*(2)*sizeof(Real_t)/(64*V_BLK_CACHE);
            if(n_blk1==0) n_blk1=1;
            size_t interac_dsp_=0;
            for(size_t blk1=0;blk1<n_blk1;blk1++){
              size_t blk1_start=(nodes_out_.size()* blk1   )/n_blk1;
              size_t blk1_end  =(nodes_out_.size()*(blk1+1))/n_blk1;
              for(size_t k=0;k<mat_cnt;k++){
                for(size_t i=blk1_start;i<blk1_end;i++){
                  Vector<FMMNode_t*>& lst=nodes_out_[i]->interac_list[interac_type];
                  if(lst[k]!=NULL && lst[k]->pt_cnt[0]){
                    interac_vec[blk0].push_back(lst[k]->node_id*fftsize*ker_dim0*dof);
                    interac_vec[blk0].push_back(    i          *fftsize*ker_dim1*dof);
                    interac_dsp_++;
                  }
                }
                interac_dsp[blk0].push_back(interac_dsp_);
              }
            }
          }
        }
      }
      {
        size_t data_size=sizeof(size_t)*6;
        for(size_t blk0=0;blk0<n_blk0;blk0++){
          data_size+=sizeof(size_t)+    fft_vec[blk0].size()*sizeof(size_t);
          data_size+=sizeof(size_t)+   ifft_vec[blk0].size()*sizeof(size_t);
          data_size+=sizeof(size_t)+    fft_scl[blk0].size()*sizeof(Real_t);
          data_size+=sizeof(size_t)+   ifft_scl[blk0].size()*sizeof(Real_t);
          data_size+=sizeof(size_t)+interac_vec[blk0].size()*sizeof(size_t);
          data_size+=sizeof(size_t)+interac_dsp[blk0].size()*sizeof(size_t);
        }
        data_size+=sizeof(size_t)+interac_mat.size()*sizeof(size_t);
        data_size+=sizeof(size_t)+interac_mat_ptr.size()*sizeof(Real_t*);
        if(data_size>interac_data.Dim(0)*interac_data.Dim(1))
          interac_data.ReInit(1,data_size);
        char* data_ptr=&interac_data[0][0];
        ((size_t*)data_ptr)[0]=buff_size; data_ptr+=sizeof(size_t);
        ((size_t*)data_ptr)[0]=        m; data_ptr+=sizeof(size_t);
        ((size_t*)data_ptr)[0]=      dof; data_ptr+=sizeof(size_t);
        ((size_t*)data_ptr)[0]= ker_dim0; data_ptr+=sizeof(size_t);
        ((size_t*)data_ptr)[0]= ker_dim1; data_ptr+=sizeof(size_t);
        ((size_t*)data_ptr)[0]=   n_blk0; data_ptr+=sizeof(size_t);
        ((size_t*)data_ptr)[0]= interac_mat.size(); data_ptr+=sizeof(size_t);
        mem::memcopy(data_ptr, &interac_mat[0], interac_mat.size()*sizeof(size_t));
        data_ptr+=interac_mat.size()*sizeof(size_t);
        ((size_t*)data_ptr)[0]= interac_mat_ptr.size(); data_ptr+=sizeof(size_t);
        mem::memcopy(data_ptr, &interac_mat_ptr[0], interac_mat_ptr.size()*sizeof(Real_t*));
        data_ptr+=interac_mat_ptr.size()*sizeof(Real_t*);
        for(size_t blk0=0;blk0<n_blk0;blk0++){
          ((size_t*)data_ptr)[0]= fft_vec[blk0].size(); data_ptr+=sizeof(size_t);
          mem::memcopy(data_ptr, & fft_vec[blk0][0],  fft_vec[blk0].size()*sizeof(size_t));
          data_ptr+= fft_vec[blk0].size()*sizeof(size_t);
          ((size_t*)data_ptr)[0]=ifft_vec[blk0].size(); data_ptr+=sizeof(size_t);
          mem::memcopy(data_ptr, &ifft_vec[blk0][0], ifft_vec[blk0].size()*sizeof(size_t));
          data_ptr+=ifft_vec[blk0].size()*sizeof(size_t);
          ((size_t*)data_ptr)[0]= fft_scl[blk0].size(); data_ptr+=sizeof(size_t);
          mem::memcopy(data_ptr, & fft_scl[blk0][0],  fft_scl[blk0].size()*sizeof(Real_t));
          data_ptr+= fft_scl[blk0].size()*sizeof(Real_t);
          ((size_t*)data_ptr)[0]=ifft_scl[blk0].size(); data_ptr+=sizeof(size_t);
          mem::memcopy(data_ptr, &ifft_scl[blk0][0], ifft_scl[blk0].size()*sizeof(Real_t));
          data_ptr+=ifft_scl[blk0].size()*sizeof(Real_t);
          ((size_t*)data_ptr)[0]=interac_vec[blk0].size(); data_ptr+=sizeof(size_t);
          mem::memcopy(data_ptr, &interac_vec[blk0][0], interac_vec[blk0].size()*sizeof(size_t));
          data_ptr+=interac_vec[blk0].size()*sizeof(size_t);
          ((size_t*)data_ptr)[0]=interac_dsp[blk0].size(); data_ptr+=sizeof(size_t);
          mem::memcopy(data_ptr, &interac_dsp[blk0][0], interac_dsp[blk0].size()*sizeof(size_t));
          data_ptr+=interac_dsp[blk0].size()*sizeof(size_t);
        }
      }
    }
    Profile::Toc();
  }

  void FFT_UpEquiv(size_t dof, size_t m, size_t ker_dim0, Vector<size_t>& fft_vec, Vector<Real_t>& fft_scal,
		   Vector<Real_t>& input_data, Vector<Real_t>& output_data, Vector<Real_t>& buffer_) {
    size_t n1=m*2;
    size_t n2=n1*n1;
    size_t n3=n1*n2;
    size_t n3_=n2*(n1/2+1);
    size_t chld_cnt=1UL<<COORD_DIM;
    size_t fftsize_in =2*n3_*chld_cnt*ker_dim0*dof;
    int omp_p=omp_get_max_threads();
    size_t n=6*(m-1)*(m-1)+2;
    static Vector<size_t> map;
    {
      size_t n_old=map.Dim();
      if(n_old!=n){
        Real_t c[3]={0,0,0};
        Vector<Real_t> surf=surface(m, c, (Real_t)(m-1), 0);
        map.Resize(surf.Dim()/COORD_DIM);
        for(size_t i=0;i<map.Dim();i++)
          map[i]=((size_t)(m-1-surf[i*3]+0.5))+((size_t)(m-1-surf[i*3+1]+0.5))*n1+((size_t)(m-1-surf[i*3+2]+0.5))*n2;
      }
    }
    {
      if(!vlist_fft_flag){
        int nnn[3]={(int)n1,(int)n1,(int)n1};
        void *fftw_in, *fftw_out;
        fftw_in  = mem::aligned_new<Real_t>(  n3 *ker_dim0*chld_cnt);
        fftw_out = mem::aligned_new<Real_t>(2*n3_*ker_dim0*chld_cnt);
        vlist_fftplan = FFTW_t<Real_t>::fft_plan_many_dft_r2c(COORD_DIM,nnn,ker_dim0*chld_cnt,
            (Real_t*)fftw_in, NULL, 1, n3, (typename FFTW_t<Real_t>::cplx*)(fftw_out),NULL, 1, n3_);
        mem::aligned_delete<Real_t>((Real_t*)fftw_in );
        mem::aligned_delete<Real_t>((Real_t*)fftw_out);
        vlist_fft_flag=true;
      }
    }
    {
      size_t n_in = fft_vec.Dim();
#pragma omp parallel for
      for(int pid=0; pid<omp_p; pid++){
        size_t node_start=(n_in*(pid  ))/omp_p;
        size_t node_end  =(n_in*(pid+1))/omp_p;
        Vector<Real_t> buffer(fftsize_in, &buffer_[fftsize_in*pid], false);
        for(size_t node_idx=node_start; node_idx<node_end; node_idx++){
          Matrix<Real_t>  upward_equiv(chld_cnt,n*ker_dim0*dof,&input_data[0] + fft_vec[node_idx],false);
          Vector<Real_t> upward_equiv_fft(fftsize_in, &output_data[fftsize_in *node_idx], false);
          upward_equiv_fft.SetZero();
          for(size_t k=0;k<n;k++){
            size_t idx=map[k];
            for(int j1=0;j1<dof;j1++)
            for(int j0=0;j0<(int)chld_cnt;j0++)
            for(int i=0;i<ker_dim0;i++)
              upward_equiv_fft[idx+(j0+(i+j1*ker_dim0)*chld_cnt)*n3]=upward_equiv[j0][ker_dim0*(n*j1+k)+i]*fft_scal[ker_dim0*node_idx+i];
          }
          for(int i=0;i<dof;i++)
            FFTW_t<Real_t>::fft_execute_dft_r2c(vlist_fftplan, (Real_t*)&upward_equiv_fft[i*  n3 *ker_dim0*chld_cnt],
                                        (typename FFTW_t<Real_t>::cplx*)&buffer          [i*2*n3_*ker_dim0*chld_cnt]);
#ifndef FFTW3_MKL
          double add, mul, fma;
          FFTW_t<Real_t>::fftw_flops(vlist_fftplan, &add, &mul, &fma);
#endif
          for(int i=0;i<ker_dim0*dof;i++)
          for(size_t j=0;j<n3_;j++)
          for(size_t k=0;k<chld_cnt;k++){
            upward_equiv_fft[2*(chld_cnt*(n3_*i+j)+k)+0]=buffer[2*(n3_*(chld_cnt*i+k)+j)+0];
            upward_equiv_fft[2*(chld_cnt*(n3_*i+j)+k)+1]=buffer[2*(n3_*(chld_cnt*i+k)+j)+1];
          }
        }
      }
    }
  }
  
  void FFT_Check2Equiv(size_t dof, size_t m, size_t ker_dim1, Vector<size_t>& ifft_vec, Vector<Real_t>& ifft_scal,
		       Vector<Real_t>& input_data, Vector<Real_t>& output_data, Vector<Real_t>& buffer_) {
    size_t n1=m*2;
    size_t n2=n1*n1;
    size_t n3=n1*n2;
    size_t n3_=n2*(n1/2+1);
    size_t chld_cnt=1UL<<COORD_DIM;
    size_t fftsize_out=2*n3_*dof*ker_dim1*chld_cnt;
    int omp_p=omp_get_max_threads();
    size_t n=6*(m-1)*(m-1)+2;
    static Vector<size_t> map;
    {
      size_t n_old=map.Dim();
      if(n_old!=n){
        Real_t c[3]={0,0,0};
        Vector<Real_t> surf=surface(m, c, (Real_t)(m-1), 0);
        map.Resize(surf.Dim()/COORD_DIM);
        for(size_t i=0;i<map.Dim();i++)
          map[i]=((size_t)(m*2-0.5-surf[i*3]))+((size_t)(m*2-0.5-surf[i*3+1]))*n1+((size_t)(m*2-0.5-surf[i*3+2]))*n2;
      }
    }
    {
      if(!vlist_ifft_flag){
        int nnn[3]={(int)n1,(int)n1,(int)n1};
        Real_t *fftw_in, *fftw_out;
        fftw_in  = mem::aligned_new<Real_t>(2*n3_*ker_dim1*chld_cnt);
        fftw_out = mem::aligned_new<Real_t>(  n3 *ker_dim1*chld_cnt);
        vlist_ifftplan = FFTW_t<Real_t>::fft_plan_many_dft_c2r(COORD_DIM,nnn,ker_dim1*chld_cnt,
            (typename FFTW_t<Real_t>::cplx*)fftw_in, NULL, 1, n3_, (Real_t*)(fftw_out),NULL, 1, n3);
        mem::aligned_delete<Real_t>(fftw_in);
        mem::aligned_delete<Real_t>(fftw_out);
        vlist_ifft_flag=true;
      }
    }
    {
      assert(buffer_.Dim()>=2*fftsize_out*omp_p);
      size_t n_out=ifft_vec.Dim();
#pragma omp parallel for
      for(int pid=0; pid<omp_p; pid++){
        size_t node_start=(n_out*(pid  ))/omp_p;
        size_t node_end  =(n_out*(pid+1))/omp_p;
        Vector<Real_t> buffer0(fftsize_out, &buffer_[fftsize_out*(2*pid+0)], false);
        Vector<Real_t> buffer1(fftsize_out, &buffer_[fftsize_out*(2*pid+1)], false);
        for(size_t node_idx=node_start; node_idx<node_end; node_idx++){
          Vector<Real_t> dnward_check_fft(fftsize_out, &input_data[fftsize_out*node_idx], false);
          Vector<Real_t> dnward_equiv(ker_dim1*n*dof*chld_cnt,&output_data[0] + ifft_vec[node_idx],false);
          for(int i=0;i<ker_dim1*dof;i++)
          for(size_t j=0;j<n3_;j++)
          for(size_t k=0;k<chld_cnt;k++){
            buffer0[2*(n3_*(ker_dim1*dof*k+i)+j)+0]=dnward_check_fft[2*(chld_cnt*(n3_*i+j)+k)+0];
            buffer0[2*(n3_*(ker_dim1*dof*k+i)+j)+1]=dnward_check_fft[2*(chld_cnt*(n3_*i+j)+k)+1];
          }
          for(int i=0;i<dof;i++)
            FFTW_t<Real_t>::fft_execute_dft_c2r(vlist_ifftplan, (typename FFTW_t<Real_t>::cplx*)&buffer0[i*2*n3_*ker_dim1*chld_cnt],
						(Real_t*)&buffer1[i*  n3 *ker_dim1*chld_cnt]);
#ifndef FFTW3_MKL
          double add, mul, fma;
          FFTW_t<Real_t>::fftw_flops(vlist_ifftplan, &add, &mul, &fma);
#endif
          for(size_t k=0;k<n;k++){
            size_t idx=map[k];
            for(int j1=0;j1<dof;j1++)
            for(int j0=0;j0<(int)chld_cnt;j0++)
            for(int i=0;i<ker_dim1;i++)
              dnward_equiv[ker_dim1*(n*(dof*j0+j1)+k)+i]+=buffer1[idx+(i+(j1+j0*dof)*ker_dim1)*n3]*ifft_scal[ker_dim1*node_idx+i];
          }
        }
      }
    }
  }
  
  void V_List(SetupData<Real_t,FMMNode_t>&  setup_data){
    if(!this->MultipoleOrder()) return;
    int np=1;
    if(setup_data.interac_data.Dim(0)==0 || setup_data.interac_data.Dim(1)==0){
      return;
    }
    Profile::Tic("Host2Device",false,25);
    int level=setup_data.level;
    size_t buff_size=*((size_t*)&setup_data.interac_data[0][0]);
    typename Vector<char>::Device          buff;
    typename Matrix<char>::Device  interac_data;
    typename Matrix<Real_t>::Device  input_data;
    typename Matrix<Real_t>::Device output_data;
    if(this->dev_buffer.Dim()<buff_size) this->dev_buffer.ReInit(buff_size);
    buff        =       this-> dev_buffer;
    interac_data= setup_data.interac_data;
    input_data  =*setup_data.  input_data;
    output_data =*setup_data. output_data;
    Profile::Toc();
    {
      size_t m, dof, ker_dim0, ker_dim1, n_blk0;
      std::vector<Vector<size_t> >  fft_vec;
      std::vector<Vector<size_t> > ifft_vec;
      std::vector<Vector<Real_t> >  fft_scl;
      std::vector<Vector<Real_t> > ifft_scl;
      std::vector<Vector<size_t> > interac_vec;
      std::vector<Vector<size_t> > interac_dsp;
      Vector<Real_t*> precomp_mat;
      {
        char* data_ptr=&interac_data[0][0];
        buff_size=((size_t*)data_ptr)[0]; data_ptr+=sizeof(size_t);
        m        =((size_t*)data_ptr)[0]; data_ptr+=sizeof(size_t);
        dof      =((size_t*)data_ptr)[0]; data_ptr+=sizeof(size_t);
        ker_dim0 =((size_t*)data_ptr)[0]; data_ptr+=sizeof(size_t);
        ker_dim1 =((size_t*)data_ptr)[0]; data_ptr+=sizeof(size_t);
        n_blk0   =((size_t*)data_ptr)[0]; data_ptr+=sizeof(size_t);
        fft_vec .resize(n_blk0);
        ifft_vec.resize(n_blk0);
        fft_scl .resize(n_blk0);
        ifft_scl.resize(n_blk0);
        interac_vec.resize(n_blk0);
        interac_dsp.resize(n_blk0);
        Vector<size_t> interac_mat;
        interac_mat.ReInit(((size_t*)data_ptr)[0],(size_t*)(data_ptr+sizeof(size_t)),false);
        data_ptr+=sizeof(size_t)+interac_mat.Dim()*sizeof(size_t);
        Vector<Real_t*> interac_mat_ptr;
        interac_mat_ptr.ReInit(((size_t*)data_ptr)[0],(Real_t**)(data_ptr+sizeof(size_t)),false);
        data_ptr+=sizeof(size_t)+interac_mat_ptr.Dim()*sizeof(Real_t*);
        precomp_mat.Resize(interac_mat_ptr.Dim());
        for(size_t i=0;i<interac_mat_ptr.Dim();i++){
          precomp_mat[i]=interac_mat_ptr[i];
        }
        for(size_t blk0=0;blk0<n_blk0;blk0++){
          fft_vec[blk0].ReInit(((size_t*)data_ptr)[0],(size_t*)(data_ptr+sizeof(size_t)),false);
          data_ptr+=sizeof(size_t)+fft_vec[blk0].Dim()*sizeof(size_t);
          ifft_vec[blk0].ReInit(((size_t*)data_ptr)[0],(size_t*)(data_ptr+sizeof(size_t)),false);
          data_ptr+=sizeof(size_t)+ifft_vec[blk0].Dim()*sizeof(size_t);
          fft_scl[blk0].ReInit(((size_t*)data_ptr)[0],(Real_t*)(data_ptr+sizeof(size_t)),false);
          data_ptr+=sizeof(size_t)+fft_scl[blk0].Dim()*sizeof(Real_t);
          ifft_scl[blk0].ReInit(((size_t*)data_ptr)[0],(Real_t*)(data_ptr+sizeof(size_t)),false);
          data_ptr+=sizeof(size_t)+ifft_scl[blk0].Dim()*sizeof(Real_t);
          interac_vec[blk0].ReInit(((size_t*)data_ptr)[0],(size_t*)(data_ptr+sizeof(size_t)),false);
          data_ptr+=sizeof(size_t)+interac_vec[blk0].Dim()*sizeof(size_t);
          interac_dsp[blk0].ReInit(((size_t*)data_ptr)[0],(size_t*)(data_ptr+sizeof(size_t)),false);
          data_ptr+=sizeof(size_t)+interac_dsp[blk0].Dim()*sizeof(size_t);
        }
      }
      int omp_p=omp_get_max_threads();
      size_t M_dim, fftsize;
      {
        size_t n1=m*2;
        size_t n2=n1*n1;
        size_t n3_=n2*(n1/2+1);
        size_t chld_cnt=1UL<<COORD_DIM;
        fftsize=2*n3_*chld_cnt;
        M_dim=n3_;
      }
      for(size_t blk0=0;blk0<n_blk0;blk0++){
        size_t n_in = fft_vec[blk0].Dim();
        size_t n_out=ifft_vec[blk0].Dim();
        size_t  input_dim=n_in *ker_dim0*dof*fftsize;
        size_t output_dim=n_out*ker_dim1*dof*fftsize;
        size_t buffer_dim=2*(ker_dim0+ker_dim1)*dof*fftsize*omp_p;
        Vector<Real_t> fft_in ( input_dim, (Real_t*)&buff[         0                           ],false);
        Vector<Real_t> fft_out(output_dim, (Real_t*)&buff[ input_dim            *sizeof(Real_t)],false);
        Vector<Real_t>  buffer(buffer_dim, (Real_t*)&buff[(input_dim+output_dim)*sizeof(Real_t)],false);
        {
          if(np==1) Profile::Tic("FFT",false,100);
          Vector<Real_t>  input_data_( input_data.dim[0]* input_data.dim[1],  input_data[0], false);
          FFT_UpEquiv(dof, m, ker_dim0,  fft_vec[blk0],  fft_scl[blk0],  input_data_, fft_in, buffer);
          if(np==1) Profile::Toc();
        }
        {
#ifdef PVFMM_HAVE_PAPI
#ifdef __VERBOSE__
          std::cout << "Starting counters new\n";
          if (PAPI_start(EventSet) != PAPI_OK) std::cout << "handle_error3" << std::endl;
#endif
#endif
          if(np==1) Profile::Tic("HadamardProduct",false,100);
          VListHadamard<Real_t>(dof, M_dim, ker_dim0, ker_dim1, interac_dsp[blk0], interac_vec[blk0], precomp_mat, fft_in, fft_out);
          if(np==1) Profile::Toc();
#ifdef PVFMM_HAVE_PAPI
#ifdef __VERBOSE__
          if (PAPI_stop(EventSet, values) != PAPI_OK) std::cout << "handle_error4" << std::endl;
          std::cout << "Stopping counters\n";
#endif
#endif
        }
        {
          if(np==1) Profile::Tic("IFFT",false,100);
          Vector<Real_t> output_data_(output_data.dim[0]*output_data.dim[1], output_data[0], false);
          FFT_Check2Equiv(dof, m, ker_dim1, ifft_vec[blk0], ifft_scl[blk0], fft_out, output_data_, buffer);
          if(np==1) Profile::Toc();
        }
      }
    }
  }

  void Down2DownSetup(SetupData<Real_t,FMMNode_t>& setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& buff, std::vector<Vector<FMMNode_t*> >& n_list, int level){
    if(!this->MultipoleOrder()) return;
    {
      setup_data.level=level;
      setup_data.kernel=kernel->k_l2l;
      setup_data.interac_type.resize(1);
      setup_data.interac_type[0]=D2D_Type;
      setup_data. input_data=&buff[1];
      setup_data.output_data=&buff[1];
      Vector<FMMNode_t*>& nodes_in =n_list[1];
      Vector<FMMNode_t*>& nodes_out=n_list[1];
      setup_data.nodes_in .clear();
      setup_data.nodes_out.clear();
      for(size_t i=0;i<nodes_in .Dim();i++) if((nodes_in [i]->depth==level-1) && nodes_in [i]->pt_cnt[1]) setup_data.nodes_in .push_back(nodes_in [i]);
      for(size_t i=0;i<nodes_out.Dim();i++) if((nodes_out[i]->depth==level  ) && nodes_out[i]->pt_cnt[1]) setup_data.nodes_out.push_back(nodes_out[i]);
    }
    std::vector<FMMNode_t*>& nodes_in =setup_data.nodes_in ;
    std::vector<FMMNode_t*>& nodes_out=setup_data.nodes_out;
    std::vector<Vector<Real_t>*>&  input_vector=setup_data. input_vector;  input_vector.clear();
    std::vector<Vector<Real_t>*>& output_vector=setup_data.output_vector; output_vector.clear();
    for(size_t i=0;i<nodes_in .size();i++)  input_vector.push_back(&(nodes_in[i]->FMMData())->dnward_equiv);
    for(size_t i=0;i<nodes_out.size();i++) output_vector.push_back(&(nodes_out[i]->FMMData())->dnward_equiv);
    SetupInterac(setup_data);
  }
  
  void Down2Down(SetupData<Real_t,FMMNode_t>& setup_data){
    if(!this->MultipoleOrder()) return;
    EvalList(setup_data);
  }

  void X_ListSetup(SetupData<Real_t,FMMNode_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& buff, std::vector<Vector<FMMNode_t*> >& n_list, int level){
    if(!this->MultipoleOrder()) return;
    {
      setup_data. level=level;
      setup_data.kernel=kernel->k_s2l;
      setup_data. input_data=&buff[4];
      setup_data.output_data=&buff[1];
      setup_data. coord_data=&buff[6];
      Vector<FMMNode_t*>& nodes_in =n_list[4];
      Vector<FMMNode_t*>& nodes_out=n_list[1];
      setup_data.nodes_in .clear();
      setup_data.nodes_out.clear();
      for(size_t i=0;i<nodes_in .Dim();i++) if((level==0 || level==-1) && (nodes_in [i]->src_coord.Dim() || nodes_in [i]->surf_coord.Dim()) &&  nodes_in [i]->IsLeaf ()) setup_data.nodes_in .push_back(nodes_in [i]);
      for(size_t i=0;i<nodes_out.Dim();i++) if((level==0 || level==-1) &&  nodes_out[i]->pt_cnt[1]                                          && !nodes_out[i]->IsGhost()) setup_data.nodes_out.push_back(nodes_out[i]);
    }
    ptSetupData data;
    data. level=setup_data. level;
    data.kernel=setup_data.kernel;
    std::vector<FMMNode_t*>& nodes_in =setup_data.nodes_in ;
    std::vector<FMMNode_t*>& nodes_out=setup_data.nodes_out;
    {
      std::vector<FMMNode_t*>& nodes=nodes_in;
      PackedData& coord=data.src_coord;
      PackedData& value=data.src_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data. input_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        ((FMMNode_t*)nodes[i])->node_id=i;
        Vector<Real_t>& coord_vec=nodes[i]->src_coord;
        Vector<Real_t>& value_vec=nodes[i]->src_value;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      std::vector<FMMNode_t*>& nodes=nodes_in;
      PackedData& coord=data.srf_coord;
      PackedData& value=data.srf_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data. input_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        Vector<Real_t>& coord_vec=nodes[i]->surf_coord;
        Vector<Real_t>& value_vec=nodes[i]->surf_value;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      std::vector<FMMNode_t*>& nodes=nodes_out;
      PackedData& coord=data.trg_coord;
      PackedData& value=data.trg_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data.output_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        Vector<Real_t>& coord_vec=tree->dnwd_check_surf[nodes[i]->depth];
        Vector<Real_t>& value_vec=(nodes[i]->FMMData())->dnward_equiv;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      int omp_p=omp_get_max_threads();
      std::vector<std::vector<size_t> > in_node_(omp_p);
      std::vector<std::vector<size_t> > scal_idx_(omp_p);
      std::vector<std::vector<Real_t> > coord_shift_(omp_p);
      std::vector<std::vector<size_t> > interac_cnt_(omp_p);
      size_t m=this->MultipoleOrder();
      size_t Nsrf=(6*(m-1)*(m-1)+2);
#pragma omp parallel for
      for(size_t tid=0;tid<omp_p;tid++){
        std::vector<size_t>& in_node    =in_node_[tid];
        std::vector<size_t>& scal_idx   =scal_idx_[tid];
        std::vector<Real_t>& coord_shift=coord_shift_[tid];
        std::vector<size_t>& interac_cnt=interac_cnt_[tid];
        size_t a=(nodes_out.size()*(tid+0))/omp_p;
        size_t b=(nodes_out.size()*(tid+1))/omp_p;
        for(size_t i=a;i<b;i++){
          FMMNode_t* tnode=nodes_out[i];
          if(tnode->IsLeaf() && tnode->pt_cnt[1]<=Nsrf){
            interac_cnt.push_back(0);
            continue;
          }
          Real_t s=pvfmm::pow<Real_t>(0.5,tnode->depth);
          size_t interac_cnt_=0;
          {
            Mat_Type type=X_Type;
            Vector<FMMNode_t*>& intlst=tnode->interac_list[type];
            for(size_t j=0;j<intlst.Dim();j++) if(intlst[j]){
              FMMNode_t* snode=intlst[j];
              size_t snode_id=snode->node_id;
              if(snode_id>=nodes_in.size() || nodes_in[snode_id]!=snode) continue;
              in_node.push_back(snode_id);
              scal_idx.push_back(snode->depth);
              {
                const int* rel_coord=interac_list.RelativeCoord(type,j);
                const Real_t* scoord=snode->Coord();
                const Real_t* tcoord=tnode->Coord();
                Real_t shift[COORD_DIM];
                shift[0]=rel_coord[0]*0.5*s-(scoord[0]+1.0*s)+(0+0.5*s);
                shift[1]=rel_coord[1]*0.5*s-(scoord[1]+1.0*s)+(0+0.5*s);
                shift[2]=rel_coord[2]*0.5*s-(scoord[2]+1.0*s)+(0+0.5*s);
                coord_shift.push_back(shift[0]);
                coord_shift.push_back(shift[1]);
                coord_shift.push_back(shift[2]);
              }
              interac_cnt_++;
            }
          }
          interac_cnt.push_back(interac_cnt_);
        }
      }
      {
        InteracData& interac_data=data.interac_data;
	CopyVec(in_node_,interac_data.in_node);
	CopyVec(scal_idx_,interac_data.scal_idx);
	CopyVec(coord_shift_,interac_data.coord_shift);
	CopyVec(interac_cnt_,interac_data.interac_cnt);
        {
          pvfmm::Vector<size_t>& cnt=interac_data.interac_cnt;
          pvfmm::Vector<size_t>& dsp=interac_data.interac_dsp;
          dsp.ReInit(cnt.Dim()); if(dsp.Dim()) dsp[0]=0;
          omp_par::scan(&cnt[0],&dsp[0],dsp.Dim());
        }
      }
    }
    PtSetup(setup_data, &data);
  }
  
  void X_List(SetupData<Real_t,FMMNode_t>&  setup_data){
    if(!this->MultipoleOrder()) return;
    this->EvalListPts(setup_data);
  }
  
  void W_ListSetup(SetupData<Real_t,FMMNode_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& buff, std::vector<Vector<FMMNode_t*> >& n_list, int level){
    if(!this->MultipoleOrder()) return;
    {
      setup_data. level=level;
      setup_data.kernel=kernel->k_m2t;
      setup_data. input_data=&buff[0];
      setup_data.output_data=&buff[5];
      setup_data. coord_data=&buff[6];
      Vector<FMMNode_t*>& nodes_in =n_list[0];
      Vector<FMMNode_t*>& nodes_out=n_list[5];
      setup_data.nodes_in .clear();
      setup_data.nodes_out.clear();
      for(size_t i=0;i<nodes_in .Dim();i++) if((level==0 || level==-1) && nodes_in [i]->pt_cnt[0]                                                            ) setup_data.nodes_in .push_back(nodes_in [i]);
      for(size_t i=0;i<nodes_out.Dim();i++) if((level==0 || level==-1) && nodes_out[i]->trg_coord.Dim() && nodes_out[i]->IsLeaf() && !nodes_out[i]->IsGhost()) setup_data.nodes_out.push_back(nodes_out[i]);
    }
    ptSetupData data;
    data. level=setup_data. level;
    data.kernel=setup_data.kernel;
    std::vector<FMMNode_t*>& nodes_in =setup_data.nodes_in ;
    std::vector<FMMNode_t*>& nodes_out=setup_data.nodes_out;
    {
      std::vector<FMMNode_t*>& nodes=nodes_in;
      PackedData& coord=data.src_coord;
      PackedData& value=data.src_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data. input_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        ((FMMNode_t*)nodes[i])->node_id=i;
        Vector<Real_t>& coord_vec=tree->upwd_equiv_surf[nodes[i]->depth];
        Vector<Real_t>& value_vec=(nodes[i]->FMMData())->upward_equiv;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      std::vector<FMMNode_t*>& nodes=nodes_in;
      PackedData& coord=data.srf_coord;
      PackedData& value=data.srf_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data. input_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        coord.dsp[i]=0;
        coord.cnt[i]=0;
        value.dsp[i]=0;
        value.cnt[i]=0;
      }
    }
    {
      std::vector<FMMNode_t*>& nodes=nodes_out;
      PackedData& coord=data.trg_coord;
      PackedData& value=data.trg_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data.output_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        Vector<Real_t>& coord_vec=nodes[i]->trg_coord;
        Vector<Real_t>& value_vec=nodes[i]->trg_value;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      int omp_p=omp_get_max_threads();
      std::vector<std::vector<size_t> > in_node_(omp_p);
      std::vector<std::vector<size_t> > scal_idx_(omp_p);
      std::vector<std::vector<Real_t> > coord_shift_(omp_p);
      std::vector<std::vector<size_t> > interac_cnt_(omp_p);
      size_t m=this->MultipoleOrder();
      size_t Nsrf=(6*(m-1)*(m-1)+2);
#pragma omp parallel for
      for(size_t tid=0;tid<omp_p;tid++){
        std::vector<size_t>& in_node    =in_node_[tid]    ;
        std::vector<size_t>& scal_idx   =scal_idx_[tid]   ;
        std::vector<Real_t>& coord_shift=coord_shift_[tid];
        std::vector<size_t>& interac_cnt=interac_cnt_[tid]        ;
        size_t a=(nodes_out.size()*(tid+0))/omp_p;
        size_t b=(nodes_out.size()*(tid+1))/omp_p;
        for(size_t i=a;i<b;i++){
          FMMNode_t* tnode=nodes_out[i];
          Real_t s=pvfmm::pow<Real_t>(0.5,tnode->depth);
          size_t interac_cnt_=0;
          {
            Mat_Type type=W_Type;
            Vector<FMMNode_t*>& intlst=tnode->interac_list[type];
            for(size_t j=0;j<intlst.Dim();j++) if(intlst[j]){
              FMMNode_t* snode=intlst[j];
              size_t snode_id=snode->node_id;
              if(snode_id>=nodes_in.size() || nodes_in[snode_id]!=snode) continue;
              if(snode->IsGhost() && snode->src_coord.Dim()+snode->surf_coord.Dim()==0){
              }else if(snode->IsLeaf() && snode->pt_cnt[0]<=Nsrf) continue;
              in_node.push_back(snode_id);
              scal_idx.push_back(snode->depth);
              {
                const int* rel_coord=interac_list.RelativeCoord(type,j);
                const Real_t* scoord=snode->Coord();
                const Real_t* tcoord=tnode->Coord();
                Real_t shift[COORD_DIM];
                shift[0]=rel_coord[0]*0.25*s-(0+0.25*s)+(tcoord[0]+0.5*s);
                shift[1]=rel_coord[1]*0.25*s-(0+0.25*s)+(tcoord[1]+0.5*s);
                shift[2]=rel_coord[2]*0.25*s-(0+0.25*s)+(tcoord[2]+0.5*s);
                coord_shift.push_back(shift[0]);
                coord_shift.push_back(shift[1]);
                coord_shift.push_back(shift[2]);
              }
              interac_cnt_++;
            }
          }
          interac_cnt.push_back(interac_cnt_);
        }
      }
      {
        InteracData& interac_data=data.interac_data;
	CopyVec(in_node_,interac_data.in_node);
	CopyVec(scal_idx_,interac_data.scal_idx);
	CopyVec(coord_shift_,interac_data.coord_shift);
	CopyVec(interac_cnt_,interac_data.interac_cnt);
        {
          pvfmm::Vector<size_t>& cnt=interac_data.interac_cnt;
          pvfmm::Vector<size_t>& dsp=interac_data.interac_dsp;
          dsp.ReInit(cnt.Dim()); if(dsp.Dim()) dsp[0]=0;
          omp_par::scan(&cnt[0],&dsp[0],dsp.Dim());
        }
      }
    }
    PtSetup(setup_data, &data);
  }
  
  void W_List(SetupData<Real_t,FMMNode_t>&  setup_data){
    if(!this->MultipoleOrder()) return;
    this->EvalListPts(setup_data);
  }  

  void U_ListSetup(SetupData<Real_t,FMMNode_t>& setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& buff, std::vector<Vector<FMMNode_t*> >& n_list, int level){
    {
      setup_data. level=level;
      setup_data.kernel=kernel->k_s2t;
      setup_data. input_data=&buff[4];
      setup_data.output_data=&buff[5];
      setup_data. coord_data=&buff[6];
      Vector<FMMNode_t*>& nodes_in =n_list[4];
      Vector<FMMNode_t*>& nodes_out=n_list[5];
      setup_data.nodes_in .clear();
      setup_data.nodes_out.clear();
      for(size_t i=0;i<nodes_in .Dim();i++)
        if((level==0 || level==-1)
  	 && (nodes_in [i]->src_coord.Dim() || nodes_in [i]->surf_coord.Dim())
  	 && nodes_in [i]->IsLeaf()                            ) setup_data.nodes_in .push_back(nodes_in [i]);
      for(size_t i=0;i<nodes_out.Dim();i++)
        if((level==0 || level==-1)
  	 && (nodes_out[i]->trg_coord.Dim()                                  )
  	 && nodes_out[i]->IsLeaf() && !nodes_out[i]->IsGhost()) setup_data.nodes_out.push_back(nodes_out[i]);
    }
    ptSetupData data;
    data. level=setup_data. level;
    data.kernel=setup_data.kernel;
    std::vector<FMMNode_t*>& nodes_in =setup_data.nodes_in ;
    std::vector<FMMNode_t*>& nodes_out=setup_data.nodes_out;
    {
      std::vector<FMMNode_t*>& nodes=nodes_in;
      PackedData& coord=data.src_coord;
      PackedData& value=data.src_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data. input_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        nodes[i]->node_id=i;
        Vector<Real_t>& coord_vec=nodes[i]->src_coord;
        Vector<Real_t>& value_vec=nodes[i]->src_value;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      std::vector<FMMNode_t*>& nodes=nodes_in;
      PackedData& coord=data.srf_coord;
      PackedData& value=data.srf_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data. input_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        Vector<Real_t>& coord_vec=nodes[i]->surf_coord;
        Vector<Real_t>& value_vec=nodes[i]->surf_value;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      std::vector<FMMNode_t*>& nodes=nodes_out;
      PackedData& coord=data.trg_coord;
      PackedData& value=data.trg_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data.output_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        Vector<Real_t>& coord_vec=nodes[i]->trg_coord;
        Vector<Real_t>& value_vec=nodes[i]->trg_value;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      int omp_p=omp_get_max_threads();
      std::vector<std::vector<size_t> > in_node_(omp_p);
      std::vector<std::vector<size_t> > scal_idx_(omp_p);
      std::vector<std::vector<Real_t> > coord_shift_(omp_p);
      std::vector<std::vector<size_t> > interac_cnt_(omp_p);
      size_t m=this->MultipoleOrder();
      size_t Nsrf=(6*(m-1)*(m-1)+2);
#pragma omp parallel for
      for(size_t tid=0;tid<omp_p;tid++){
        std::vector<size_t>& in_node    =in_node_[tid]    ;
        std::vector<size_t>& scal_idx   =scal_idx_[tid]   ;
        std::vector<Real_t>& coord_shift=coord_shift_[tid];
        std::vector<size_t>& interac_cnt=interac_cnt_[tid]        ;
        size_t a=(nodes_out.size()*(tid+0))/omp_p;
        size_t b=(nodes_out.size()*(tid+1))/omp_p;
        for(size_t i=a;i<b;i++){
          FMMNode_t* tnode=nodes_out[i];
          Real_t s=pvfmm::pow<Real_t>(0.5,tnode->depth);
          size_t interac_cnt_=0;
          {
            Mat_Type type=U0_Type;
            Vector<FMMNode_t*>& intlst=tnode->interac_list[type];
            for(size_t j=0;j<intlst.Dim();j++) if(intlst[j]){
              FMMNode_t* snode=intlst[j];
              size_t snode_id=snode->node_id;
              if(snode_id>=nodes_in.size() || nodes_in[snode_id]!=snode) continue;
              in_node.push_back(snode_id);
              scal_idx.push_back(snode->depth);
              {
                const int* rel_coord=interac_list.RelativeCoord(type,j);
                const Real_t* scoord=snode->Coord();
                const Real_t* tcoord=tnode->Coord();
                Real_t shift[COORD_DIM];
                shift[0]=rel_coord[0]*0.5*s-(scoord[0]+1.0*s)+(tcoord[0]+0.5*s);
                shift[1]=rel_coord[1]*0.5*s-(scoord[1]+1.0*s)+(tcoord[1]+0.5*s);
                shift[2]=rel_coord[2]*0.5*s-(scoord[2]+1.0*s)+(tcoord[2]+0.5*s);
                coord_shift.push_back(shift[0]);
                coord_shift.push_back(shift[1]);
                coord_shift.push_back(shift[2]);
              }
              interac_cnt_++;
            }
          }
          {
            Mat_Type type=U1_Type;
            Vector<FMMNode_t*>& intlst=tnode->interac_list[type];
            for(size_t j=0;j<intlst.Dim();j++) if(intlst[j]){
              FMMNode_t* snode=intlst[j];
              size_t snode_id=snode->node_id;
              if(snode_id>=nodes_in.size() || nodes_in[snode_id]!=snode) continue;
              in_node.push_back(snode_id);
              scal_idx.push_back(snode->depth);
              {
                const int* rel_coord=interac_list.RelativeCoord(type,j);
                const Real_t* scoord=snode->Coord();
                const Real_t* tcoord=tnode->Coord();
                Real_t shift[COORD_DIM];
                shift[0]=rel_coord[0]*1.0*s-(scoord[0]+0.5*s)+(tcoord[0]+0.5*s);
                shift[1]=rel_coord[1]*1.0*s-(scoord[1]+0.5*s)+(tcoord[1]+0.5*s);
                shift[2]=rel_coord[2]*1.0*s-(scoord[2]+0.5*s)+(tcoord[2]+0.5*s);
                coord_shift.push_back(shift[0]);
                coord_shift.push_back(shift[1]);
                coord_shift.push_back(shift[2]);
              }
              interac_cnt_++;
            }
          }
          {
            Mat_Type type=U2_Type;
            Vector<FMMNode_t*>& intlst=tnode->interac_list[type];
            for(size_t j=0;j<intlst.Dim();j++) if(intlst[j]){
              FMMNode_t* snode=intlst[j];
              size_t snode_id=snode->node_id;
              if(snode_id>=nodes_in.size() || nodes_in[snode_id]!=snode) continue;
              in_node.push_back(snode_id);
              scal_idx.push_back(snode->depth);
              {
                const int* rel_coord=interac_list.RelativeCoord(type,j);
                const Real_t* scoord=snode->Coord();
                const Real_t* tcoord=tnode->Coord();
                Real_t shift[COORD_DIM];
                shift[0]=rel_coord[0]*0.25*s-(scoord[0]+0.25*s)+(tcoord[0]+0.5*s);
                shift[1]=rel_coord[1]*0.25*s-(scoord[1]+0.25*s)+(tcoord[1]+0.5*s);
                shift[2]=rel_coord[2]*0.25*s-(scoord[2]+0.25*s)+(tcoord[2]+0.5*s);
                coord_shift.push_back(shift[0]);
                coord_shift.push_back(shift[1]);
                coord_shift.push_back(shift[2]);
              }
              interac_cnt_++;
            }
          }
          {
            Mat_Type type=X_Type;
            Vector<FMMNode_t*>& intlst=tnode->interac_list[type];
            if(tnode->pt_cnt[1]<=Nsrf)
            for(size_t j=0;j<intlst.Dim();j++) if(intlst[j]){
              FMMNode_t* snode=intlst[j];
              size_t snode_id=snode->node_id;
              if(snode_id>=nodes_in.size() || nodes_in[snode_id]!=snode) continue;
              in_node.push_back(snode_id);
              scal_idx.push_back(snode->depth);
              {
                const int* rel_coord=interac_list.RelativeCoord(type,j);
                const Real_t* scoord=snode->Coord();
                const Real_t* tcoord=tnode->Coord();
                Real_t shift[COORD_DIM];
                shift[0]=rel_coord[0]*0.5*s-(scoord[0]+1.0*s)+(tcoord[0]+0.5*s);
                shift[1]=rel_coord[1]*0.5*s-(scoord[1]+1.0*s)+(tcoord[1]+0.5*s);
                shift[2]=rel_coord[2]*0.5*s-(scoord[2]+1.0*s)+(tcoord[2]+0.5*s);
                coord_shift.push_back(shift[0]);
                coord_shift.push_back(shift[1]);
                coord_shift.push_back(shift[2]);
              }
              interac_cnt_++;
            }
          }
          {
            Mat_Type type=W_Type;
            Vector<FMMNode_t*>& intlst=tnode->interac_list[type];
            for(size_t j=0;j<intlst.Dim();j++) if(intlst[j]){
              FMMNode_t* snode=intlst[j];
              size_t snode_id=snode->node_id;
              if(snode_id>=nodes_in.size() || nodes_in[snode_id]!=snode) continue;
              if(snode->IsGhost() && snode->src_coord.Dim()+snode->surf_coord.Dim()==0) continue;
              if(snode->pt_cnt[0]> Nsrf) continue;
              in_node.push_back(snode_id);
              scal_idx.push_back(snode->depth);
              {
                const int* rel_coord=interac_list.RelativeCoord(type,j);
                const Real_t* scoord=snode->Coord();
                const Real_t* tcoord=tnode->Coord();
                Real_t shift[COORD_DIM];
                shift[0]=rel_coord[0]*0.25*s-(scoord[0]+0.25*s)+(tcoord[0]+0.5*s);
                shift[1]=rel_coord[1]*0.25*s-(scoord[1]+0.25*s)+(tcoord[1]+0.5*s);
                shift[2]=rel_coord[2]*0.25*s-(scoord[2]+0.25*s)+(tcoord[2]+0.5*s);
                coord_shift.push_back(shift[0]);
                coord_shift.push_back(shift[1]);
                coord_shift.push_back(shift[2]);
              }
              interac_cnt_++;
            }
          }
          interac_cnt.push_back(interac_cnt_);
        }
      }
      {
        InteracData& interac_data=data.interac_data;
	CopyVec(in_node_,interac_data.in_node);
	CopyVec(scal_idx_,interac_data.scal_idx);
	CopyVec(coord_shift_,interac_data.coord_shift);
	CopyVec(interac_cnt_,interac_data.interac_cnt);
        {
          pvfmm::Vector<size_t>& cnt=interac_data.interac_cnt;
          pvfmm::Vector<size_t>& dsp=interac_data.interac_dsp;
          dsp.ReInit(cnt.Dim()); if(dsp.Dim()) dsp[0]=0;
          omp_par::scan(&cnt[0],&dsp[0],dsp.Dim());
        }
      }
    }
    PtSetup(setup_data, &data);
  }

  void U_List(SetupData<Real_t,FMMNode_t>&  setup_data){
    this->EvalListPts(setup_data);
  }
  
  void Down2TargetSetup(SetupData<Real_t,FMMNode_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& buff, std::vector<Vector<FMMNode_t*> >& n_list, int level){
    if(!this->MultipoleOrder()) return;
    {
      setup_data. level=level;
      setup_data.kernel=kernel->k_l2t;
      setup_data. input_data=&buff[1];
      setup_data.output_data=&buff[5];
      setup_data. coord_data=&buff[6];
      Vector<FMMNode_t*>& nodes_in =n_list[1];
      Vector<FMMNode_t*>& nodes_out=n_list[5];
      setup_data.nodes_in .clear();
      setup_data.nodes_out.clear();
      for(size_t i=0;i<nodes_in .Dim();i++) if((nodes_in [i]->depth==level || level==-1) && nodes_in [i]->trg_coord.Dim() && nodes_in [i]->IsLeaf() && !nodes_in [i]->IsGhost()) setup_data.nodes_in .push_back(nodes_in [i]);
      for(size_t i=0;i<nodes_out.Dim();i++) if((nodes_out[i]->depth==level || level==-1) && nodes_out[i]->trg_coord.Dim() && nodes_out[i]->IsLeaf() && !nodes_out[i]->IsGhost()) setup_data.nodes_out.push_back(nodes_out[i]);
    }
    ptSetupData data;
    data. level=setup_data. level;
    data.kernel=setup_data.kernel;
    std::vector<FMMNode_t*>& nodes_in =setup_data.nodes_in ;
    std::vector<FMMNode_t*>& nodes_out=setup_data.nodes_out;
    {
      std::vector<FMMNode_t*>& nodes=nodes_in;
      PackedData& coord=data.src_coord;
      PackedData& value=data.src_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data. input_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        nodes[i]->node_id=i;
        Vector<Real_t>& coord_vec=tree->dnwd_equiv_surf[nodes[i]->depth];
        Vector<Real_t>& value_vec=(nodes[i]->FMMData())->dnward_equiv;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      std::vector<FMMNode_t*>& nodes=nodes_in;
      PackedData& coord=data.srf_coord;
      PackedData& value=data.srf_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data. input_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        coord.dsp[i]=0;
        coord.cnt[i]=0;
        value.dsp[i]=0;
        value.cnt[i]=0;
      }
    }
    {
      std::vector<FMMNode_t*>& nodes=nodes_out;
      PackedData& coord=data.trg_coord;
      PackedData& value=data.trg_value;
      coord.ptr=setup_data. coord_data;
      value.ptr=setup_data.output_data;
      coord.len=coord.ptr->Dim(0)*coord.ptr->Dim(1);
      value.len=value.ptr->Dim(0)*value.ptr->Dim(1);
      coord.cnt.ReInit(nodes.size());
      coord.dsp.ReInit(nodes.size());
      value.cnt.ReInit(nodes.size());
      value.dsp.ReInit(nodes.size());
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++){
        Vector<Real_t>& coord_vec=nodes[i]->trg_coord;
        Vector<Real_t>& value_vec=nodes[i]->trg_value;
        if(coord_vec.Dim()){
          coord.dsp[i]=&coord_vec[0]-coord.ptr[0][0];
          assert(coord.dsp[i]<coord.len);
          coord.cnt[i]=coord_vec.Dim();
        }else{
          coord.dsp[i]=0;
          coord.cnt[i]=0;
        }
        if(value_vec.Dim()){
          value.dsp[i]=&value_vec[0]-value.ptr[0][0];
          assert(value.dsp[i]<value.len);
          value.cnt[i]=value_vec.Dim();
        }else{
          value.dsp[i]=0;
          value.cnt[i]=0;
        }
      }
    }
    {
      int omp_p=omp_get_max_threads();
      std::vector<std::vector<size_t> > in_node_(omp_p);
      std::vector<std::vector<size_t> > scal_idx_(omp_p);
      std::vector<std::vector<Real_t> > coord_shift_(omp_p);
      std::vector<std::vector<size_t> > interac_cnt_(omp_p);
      if(this->ScaleInvar()){
        const Kernel<Real_t>* ker=kernel->k_l2l;
        for(size_t l=0;l<MAX_DEPTH;l++){
          Vector<Real_t>& scal=data.interac_data.scal[l*4+0];
          Vector<Real_t>& scal_exp=ker->trg_scal;
          scal.ReInit(scal_exp.Dim());
          for(size_t i=0;i<scal.Dim();i++){
            scal[i]=pvfmm::pow<Real_t>(2.0,-scal_exp[i]*l);
          }
        }
        for(size_t l=0;l<MAX_DEPTH;l++){
          Vector<Real_t>& scal=data.interac_data.scal[l*4+1];
          Vector<Real_t>& scal_exp=ker->src_scal;
          scal.ReInit(scal_exp.Dim());
          for(size_t i=0;i<scal.Dim();i++){
            scal[i]=pvfmm::pow<Real_t>(2.0,-scal_exp[i]*l);
          }
        }
      }
#pragma omp parallel for
      for(size_t tid=0;tid<omp_p;tid++){
        std::vector<size_t>& in_node    =in_node_[tid]    ;
        std::vector<size_t>& scal_idx   =scal_idx_[tid]   ;
        std::vector<Real_t>& coord_shift=coord_shift_[tid];
        std::vector<size_t>& interac_cnt=interac_cnt_[tid];
        size_t a=(nodes_out.size()*(tid+0))/omp_p;
        size_t b=(nodes_out.size()*(tid+1))/omp_p;
        for(size_t i=a;i<b;i++){
          FMMNode_t* tnode=nodes_out[i];
          Real_t s=pvfmm::pow<Real_t>(0.5,tnode->depth);
          size_t interac_cnt_=0;
          {
            Mat_Type type=D2T_Type;
            Vector<FMMNode_t*>& intlst=tnode->interac_list[type];
            for(size_t j=0;j<intlst.Dim();j++) if(intlst[j]){
              FMMNode_t* snode=intlst[j];
              size_t snode_id=snode->node_id;
              if(snode_id>=nodes_in.size() || nodes_in[snode_id]!=snode) continue;
              in_node.push_back(snode_id);
              scal_idx.push_back(snode->depth);
              {
                const int* rel_coord=interac_list.RelativeCoord(type,j);
                const Real_t* scoord=snode->Coord();
                const Real_t* tcoord=tnode->Coord();
                Real_t shift[COORD_DIM];
                shift[0]=rel_coord[0]*0.5*s-(0+0.5*s)+(tcoord[0]+0.5*s);
                shift[1]=rel_coord[1]*0.5*s-(0+0.5*s)+(tcoord[1]+0.5*s);
                shift[2]=rel_coord[2]*0.5*s-(0+0.5*s)+(tcoord[2]+0.5*s);
                coord_shift.push_back(shift[0]);
                coord_shift.push_back(shift[1]);
                coord_shift.push_back(shift[2]);
              }
              interac_cnt_++;
            }
          }
          interac_cnt.push_back(interac_cnt_);
        }
      }
      {
        InteracData& interac_data=data.interac_data;
	CopyVec(in_node_,interac_data.in_node);
	CopyVec(scal_idx_,interac_data.scal_idx);
	CopyVec(coord_shift_,interac_data.coord_shift);
	CopyVec(interac_cnt_,interac_data.interac_cnt);
        {
          pvfmm::Vector<size_t>& cnt=interac_data.interac_cnt;
          pvfmm::Vector<size_t>& dsp=interac_data.interac_dsp;
          dsp.ReInit(cnt.Dim()); if(dsp.Dim()) dsp[0]=0;
          omp_par::scan(&cnt[0],&dsp[0],dsp.Dim());
        }
      }
      {
        InteracData& interac_data=data.interac_data;
        pvfmm::Vector<size_t>& cnt=interac_data.interac_cnt;
        pvfmm::Vector<size_t>& dsp=interac_data.interac_dsp;
        if(cnt.Dim() && cnt[cnt.Dim()-1]+dsp[dsp.Dim()-1]){
          data.interac_data.M[0]=this->mat->Mat(level, DC2DE0_Type, 0);
          data.interac_data.M[1]=this->mat->Mat(level, DC2DE1_Type, 0);
        }else{
          data.interac_data.M[0].ReInit(0,0);
          data.interac_data.M[1].ReInit(0,0);
        }
      }
    }
    PtSetup(setup_data, &data);
  }

  void Down2Target(SetupData<Real_t,FMMNode_t>&  setup_data){
    if(!this->MultipoleOrder()) return;
    this->EvalListPts(setup_data);
  }
    
  void PostProcessing(FMMTree_t* tree, std::vector<FMMNode_t*>& nodes, BoundaryType bndry=FreeSpace){
    if(kernel->k_m2l->vol_poten && bndry==Periodic){
      const Kernel<Real_t>& k_m2t=*kernel->k_m2t;
      int ker_dim[2]={k_m2t.ker_dim[0],k_m2t.ker_dim[1]};
      Vector<Real_t>& up_equiv=(tree->RootNode()->FMMData())->upward_equiv;
      Matrix<Real_t> avg_density(1,ker_dim[0]); avg_density.SetZero();
      for(size_t i0=0;i0<up_equiv.Dim();i0+=ker_dim[0]){
        for(size_t i1=0;i1<ker_dim[0];i1++){
          avg_density[0][i1]+=up_equiv[i0+i1];
        }
      }
      int omp_p=omp_get_max_threads();
      std::vector<Matrix<Real_t> > M_tmp(omp_p);
#pragma omp parallel for
      for(size_t i=0;i<nodes.size();i++)
      if(nodes[i]->IsLeaf() && !nodes[i]->IsGhost()){
        Vector<Real_t>& trg_coord=nodes[i]->trg_coord;
        Vector<Real_t>& trg_value=nodes[i]->trg_value;
        size_t n_trg=trg_coord.Dim()/COORD_DIM;
        Matrix<Real_t>& M_vol=M_tmp[omp_get_thread_num()];
        M_vol.ReInit(ker_dim[0],n_trg*ker_dim[1]); M_vol.SetZero();
        k_m2t.vol_poten(&trg_coord[0],n_trg,&M_vol[0][0]);
        Matrix<Real_t> M_trg(1,n_trg*ker_dim[1],&trg_value[0],false);
        M_trg-=avg_density*M_vol;
      }
    }
  }      
  
  void DownwardPass() {
    Profile::Tic("Setup",true,3);
    std::vector<FMMNode_t*> leaf_nodes;
    int max_depth=0;
    int max_depth_loc=0;
    std::vector<FMMNode_t*>& nodes=this->GetNodeList();
    for(size_t i=0;i<nodes.size();i++){
      FMMNode_t* n=nodes[i];
      if(!n->IsGhost() && n->IsLeaf()) leaf_nodes.push_back(n);
      if(n->depth>max_depth_loc) max_depth_loc=n->depth;
    }
    max_depth = max_depth_loc;
    Profile::Toc();
    if(bndry==Periodic) {
      Profile::Tic("BoundaryCondition",false,5);
      this->PeriodicBC(dynamic_cast<FMMNode_t*>(this->RootNode()));
      Profile::Toc();
    }
    for(size_t i=0; i<=(this->ScaleInvar()?0:max_depth); i++) {
      if(!this->ScaleInvar()) {
        std::stringstream level_str;
        level_str<<"Level-"<<std::setfill('0')<<std::setw(2)<<i<<"\0";
        Profile::Tic(level_str.str().c_str(),false,5);
        Profile::Tic("Precomp",false,5);
	{Profile::Tic("Precomp-U",false,10);
        this->SetupPrecomp(setup_data[i+MAX_DEPTH*0]);
        Profile::Toc();}
        {Profile::Tic("Precomp-W",false,10);
        this->SetupPrecomp(setup_data[i+MAX_DEPTH*1]);
        Profile::Toc();}
        {Profile::Tic("Precomp-X",false,10);
        this->SetupPrecomp(setup_data[i+MAX_DEPTH*2]);
        Profile::Toc();}
        if(0){
          Profile::Tic("Precomp-V",false,10);
          this->SetupPrecomp(setup_data[i+MAX_DEPTH*3]);
          Profile::Toc();
        }
	Profile::Toc();
      }
      {Profile::Tic("X-List",false,5);
      this->X_List(setup_data[i+MAX_DEPTH*2]);
      Profile::Toc();}
      {Profile::Tic("W-List",false,5);
      this->W_List(setup_data[i+MAX_DEPTH*1]);
      Profile::Toc();}
      {Profile::Tic("U-List",false,5);
      this->U_List(setup_data[i+MAX_DEPTH*0]);
      Profile::Toc();}
      {Profile::Tic("V-List",false,5);
      this->V_List(setup_data[i+MAX_DEPTH*3]);
      Profile::Toc();}
      if(!this->ScaleInvar()){
        Profile::Toc();
      }
    }
    Profile::Tic("D2D",false,5);
    for(size_t i=0; i<=max_depth; i++) {
      if(!this->ScaleInvar()) this->SetupPrecomp(setup_data[i+MAX_DEPTH*4]);
      this->Down2Down(setup_data[i+MAX_DEPTH*4]);
    }
    Profile::Toc();
    Profile::Tic("D2T",false,5);
    for(int i=0; i<=(this->ScaleInvar()?0:max_depth); i++) {
      if(!this->ScaleInvar()) this->SetupPrecomp(setup_data[i+MAX_DEPTH*5]);
      this->Down2Target(setup_data[i+MAX_DEPTH*5]);
    }
    Profile::Toc();
    Profile::Tic("PostProc",false,5);
    this->PostProcessing(this, leaf_nodes, bndry);
    Profile::Toc();
  }

};

}//end namespace

#endif //_PVFMM_FMM_TREE_HPP_


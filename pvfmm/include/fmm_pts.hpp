#include <string>
#include <vector>
#include <cstdlib>

#include <pvfmm_common.hpp>
#include <interac_list.hpp>
#include <precomp_mat.hpp>
#include <fft_wrapper.hpp>
#include <mpi_tree.hpp>
#include <mpi_node.hpp>
#include <mem_mgr.hpp>
#include <vector.hpp>
#include <matrix.hpp>
#include <kernel.hpp>

#ifndef _PVFMM_FMM_PTS_HPP_
#define _PVFMM_FMM_PTS_HPP_

namespace pvfmm{

/**
 * \brief This class contains FMM specific data that each node contains
 * along with the functions for manipulating the data.
 */
template <class Real_t>
class FMM_Data{

 public:

  virtual ~FMM_Data(){}

  virtual FMM_Data* NewData(){return mem::aligned_new<FMM_Data>();}

  virtual void Clear();

  Vector<Real_t> upward_equiv;
  Vector<Real_t> dnward_equiv;
};


template <class Real_t>
struct SetupData{
  int level;
  const Kernel<Real_t>* kernel;
  std::vector<Mat_Type> interac_type;

  std::vector<void*> nodes_in ;
  std::vector<void*> nodes_out;
  std::vector<Vector<Real_t>*>  input_vector;
  std::vector<Vector<Real_t>*> output_vector;

  //#####################################################

  Matrix< char>  interac_data;
  Matrix< char>* precomp_data;
  Matrix<Real_t>*  coord_data;
  Matrix<Real_t>*  input_data;
  Matrix<Real_t>* output_data;
};

template <class FMM_Mat_t>
class FMM_Tree;

template <class FMMNode>
class FMM_Pts{

 public:

  typedef FMM_Tree<FMM_Pts<FMMNode> > FMMTree_t;
  typedef typename FMMNode::Real_t Real_t;
  typedef FMMNode FMMNode_t;

  class FMMData: public FMM_Data<Real_t>{

   public:

    virtual ~FMMData(){}

    virtual FMM_Data<Real_t>* NewData(){return mem::aligned_new<FMMData>();}
  };

  /**
   * \brief Constructor.
   */
  FMM_Pts(mem::MemoryManager* mem_mgr_=NULL): mem_mgr(mem_mgr_),
             vprecomp_fft_flag(false), vlist_fft_flag(false),
               vlist_ifft_flag(false), mat(NULL), kernel(NULL){};

  /**
   * \brief Virtual destructor.
   */
  virtual ~FMM_Pts();

  /**
   * \brief Initialize all the translation matrices (or load from file).
   * \param[in] mult_order Order of multipole expansion.
   * \param[in] kernel Kernel functions and related data.
   */
  void Initialize(int mult_order, const Kernel<Real_t>* kernel);

  /**
   * \brief Order for the multipole expansion.
   */
  int MultipoleOrder(){return multipole_order;}

  /**
   * \brief Whether using scale-invariant kernel?
   */
  bool ScaleInvar(){return kernel->scale_invar;}

  virtual void CollectNodeData(FMMTree_t* tree, std::vector<FMMNode*>& nodes, std::vector<Matrix<Real_t> >& buff, std::vector<Vector<FMMNode_t*> >& n_list, std::vector<std::vector<Vector<Real_t>* > > vec_list = std::vector<std::vector<Vector<Real_t>* > >(0));

  void SetupPrecomp(SetupData<Real_t>& setup_data);
  void SetupInterac(SetupData<Real_t>& setup_data);
  void EvalList    (SetupData<Real_t>& setup_data);
  void PtSetup(SetupData<Real_t>&  setup_data, void* data_);
  void EvalListPts(SetupData<Real_t>& setup_data);

  /**
   * \brief Initialize multipole expansions for the given array of leaf nodes
   * at a given level.
   */
  virtual void Source2UpSetup(SetupData<Real_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& node_data, std::vector<Vector<FMMNode_t*> >& n_list, int level);
  virtual void Source2Up     (SetupData<Real_t>&  setup_data);

  /**
   * \brief Initialize multipole expansions for the given array of non-leaf
   * nodes from that of its children.
   */
  virtual void Up2UpSetup(SetupData<Real_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& node_data, std::vector<Vector<FMMNode_t*> >& n_list, int level);
  virtual void Up2Up     (SetupData<Real_t>&  setup_data);

  virtual void PeriodicBC(FMMNode* node);

  /**
   * \brief Compute V-List intractions.
   */
  virtual void V_ListSetup(SetupData<Real_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& node_data, std::vector<Vector<FMMNode_t*> >& n_list, int level);
  virtual void V_List     (SetupData<Real_t>&  setup_data);

  /**
   * \brief Compute X-List intractions.
   */
  virtual void X_ListSetup(SetupData<Real_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& node_data, std::vector<Vector<FMMNode_t*> >& n_list, int level);
  virtual void X_List     (SetupData<Real_t>&  setup_data);

  /**
   * \brief Compute contribution of local expansion from the parent.
   */
  virtual void Down2DownSetup(SetupData<Real_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& node_data, std::vector<Vector<FMMNode_t*> >& n_list, int level);
  virtual void Down2Down     (SetupData<Real_t>&  setup_data);

  /**
   * \brief Compute target potential from the local expansion.
   */
  virtual void Down2TargetSetup(SetupData<Real_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& node_data, std::vector<Vector<FMMNode_t*> >& n_list, int level);
  virtual void Down2Target     (SetupData<Real_t>&  setup_data);

  /**
   * \brief Compute W-List intractions.
   */
  virtual void W_ListSetup(SetupData<Real_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& node_data, std::vector<Vector<FMMNode_t*> >& n_list, int level);
  virtual void W_List     (SetupData<Real_t>&  setup_data);

  /**
   * \brief Compute U-List intractions.
   */
  virtual void U_ListSetup(SetupData<Real_t>&  setup_data, FMMTree_t* tree, std::vector<Matrix<Real_t> >& node_data, std::vector<Vector<FMMNode_t*> >& n_list, int level);
  virtual void U_List     (SetupData<Real_t>&  setup_data);

  virtual void PostProcessing(FMMTree_t* tree, std::vector<FMMNode_t*>& nodes, BoundaryType bndry=FreeSpace);

  Vector<char> dev_buffer;
  Vector<char> staging_buffer;

 protected:

  virtual void PrecompAll(Mat_Type type, int level=-1);

  virtual Permutation<Real_t>& PrecompPerm(Mat_Type type, Perm_Type perm_indx);

  virtual Matrix<Real_t>& Precomp(int level, Mat_Type type, size_t mat_indx);
  typename FFTW_t<Real_t>::plan vprecomp_fftplan; bool vprecomp_fft_flag;

  void FFT_UpEquiv(size_t dof, size_t m, size_t ker_dim0, Vector<size_t>& fft_vec, Vector<Real_t>& fft_scl,
      Vector<Real_t>& input_data, Vector<Real_t>& output_data, Vector<Real_t>& buffer_);
  typename FFTW_t<Real_t>::plan vlist_fftplan; bool vlist_fft_flag;

  void FFT_Check2Equiv(size_t dof, size_t m, size_t ker_dim0, Vector<size_t>& ifft_vec, Vector<Real_t>& ifft_scl,
      Vector<Real_t>& input_data, Vector<Real_t>& output_data, Vector<Real_t>& buffer_);
  typename FFTW_t<Real_t>::plan vlist_ifftplan; bool vlist_ifft_flag;

  mem::MemoryManager* mem_mgr;
  InteracList<FMMNode> interac_list;
  const Kernel<Real_t>* kernel;    //The kernel function.
  PrecompMat<Real_t>* mat;   //Handles storage of matrices.
  std::string mat_fname;
  int multipole_order;       //Order of multipole expansion.

};

}//end namespace

#include <fmm_pts.txx>

#endif //_PVFMM_FMM_PTS_HPP_


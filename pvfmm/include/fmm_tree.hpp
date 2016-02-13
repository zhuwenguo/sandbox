#ifndef _PVFMM_FMM_TREE_HPP_
#define _PVFMM_FMM_TREE_HPP_

#include <fmm_pts.hpp>

namespace pvfmm{

template <class FMMNode_t>
class FMM_Tree : public FMM_Pts<FMMNode_t> {

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

 public:

  typedef typename FMM_Pts<FMMNode_t>::FMMData FMMData_t;
  typedef FMM_Tree FMMTree_t;
  using FMM_Pts<FMMNode_t>::kernel;

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


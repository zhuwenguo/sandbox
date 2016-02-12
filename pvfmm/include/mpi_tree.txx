namespace pvfmm{


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
  if(complete)
  while(curr_node<last_node){
    while( curr_node.NextId() > last_node && curr_node.GetDepth() < maxDepth-1 )
      curr_node = curr_node.getDFD(curr_node.GetDepth()+1);
    leaves_lst.push_back(curr_node);
    curr_node = curr_node.NextId();
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

template <class TreeNode>
void MPI_Tree<TreeNode>::Initialize(typename TreeNode::NodeData* init_data){
  //Initialize root node.
  Profile::Tic("InitRoot",false,5);
  Tree<TreeNode>::Initialize(init_data);
  TreeNode* rnode=this->RootNode();
  assert(this->dim==COORD_DIM);
  Profile::Toc();

  Profile::Tic("Points2Octee",true,5);
  Vector<MortonId> lin_oct;
  { //Get the linear tree.
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
  { // Sort and partition point coordinates and values.
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
      { // Compute MortonId from pt_c.
        size_t pt_cnt=pt_c.Dim()/this->dim;
        pt_mid.Resize(pt_cnt);
        #pragma omp parallel for
        for(size_t i=0;i<pt_cnt;i++){
          pt_mid[i]=MortonId(pt_c[i*COORD_DIM+0],pt_c[i*COORD_DIM+1],pt_c[i*COORD_DIM+2],this->max_depth);
        }
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

  //Initialize the pointer based tree from the linear tree.
  Profile::Tic("PointerTree",false,5);
  { // Construct the pointer tree from lin_oct
    int omp_p=omp_get_max_threads();

    // Partition nodes between threads
    rnode->SetGhost(false);
    for(int i=0;i<omp_p;i++){
      size_t idx=(lin_oct.Dim()*i)/omp_p;
      TreeNode* n=FindNode(lin_oct[idx], true);
      assert(n->GetMortonId()==lin_oct[idx]);
      UNUSED(n);
    }

    #pragma omp parallel for
    for(int i=0;i<omp_p;i++){
      size_t a=(lin_oct.Dim()* i   )/omp_p;
      size_t b=(lin_oct.Dim()*(i+1))/omp_p;

      size_t idx=a;
      TreeNode* n=FindNode(lin_oct[idx], false);
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
      //assert(idx==b); // TODO: Check why this fails
    }
  }
  Profile::Toc();
}


template <class TreeNode>
TreeNode* MPI_Tree<TreeNode>::FindNode(MortonId& key, bool subdiv,  TreeNode* start){
  int num_child=1UL<<this->Dim();
  TreeNode* n=start;
  if(n==NULL) n=this->RootNode();
  while(n->GetMortonId()<key && (!n->IsLeaf()||subdiv)){
    if(n->IsLeaf() && !n->IsGhost()) n->Subdivide();
    if(n->IsLeaf()) break;
    for(int j=0;j<num_child;j++){
      if(((TreeNode*)n->Child(j))->GetMortonId().NextId()>key){
        n=(TreeNode*)n->Child(j);
        break;
      }
    }
  }
  assert(!subdiv || n->IsGhost() || n->GetMortonId()==key);
  return n;
}


template <class TreeNode>
void MPI_Tree<TreeNode>::SetColleagues(BoundaryType bndry, TreeNode* node){
  int n1=(int)pvfmm::pow<unsigned int>(3,this->Dim());
  int n2=(int)pvfmm::pow<unsigned int>(2,this->Dim());

  if(node==NULL){
    TreeNode* curr_node=this->PreorderFirst();
    if(curr_node!=NULL){
      if(bndry==Periodic){
        for(int i=0;i<n1;i++)
          curr_node->SetColleague(curr_node,i);
      }else{
        curr_node->SetColleague(curr_node,(n1-1)/2);
      }
      curr_node=this->PreorderNxt(curr_node);
    }

    Vector<std::vector<TreeNode*> > nodes(MAX_DEPTH);
    while(curr_node!=NULL){
      nodes[curr_node->depth].push_back(curr_node);
      curr_node=this->PreorderNxt(curr_node);
    }
    for(size_t i=0;i<MAX_DEPTH;i++){
      size_t j0=nodes[i].size();
      TreeNode** nodes_=&nodes[i][0];
      #pragma omp parallel for
      for(size_t j=0;j<j0;j++){
        SetColleagues(bndry, nodes_[j]);
      }
    }

  }else{
    TreeNode* parent_node;
    TreeNode* tmp_node1;
    TreeNode* tmp_node2;

    for(int i=0;i<n1;i++)node->SetColleague(NULL,i);
    parent_node=(TreeNode*)node->Parent();
    if(parent_node==NULL) return;

    int l=node->Path2Node();
    for(int i=0;i<n1;i++){ //For each coll of the parent
      tmp_node1=(TreeNode*)parent_node->Colleague(i);
      if(tmp_node1!=NULL)
      if(!tmp_node1->IsLeaf()){
        for(int j=0;j<n2;j++){ //For each child
          tmp_node2=(TreeNode*)tmp_node1->Child(j);
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
    }// */
  }
}

}//end namespace

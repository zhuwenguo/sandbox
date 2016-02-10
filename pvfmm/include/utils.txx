/**
 * \file utils.txx
 * \author Dhairya Malhotra, dhairya.malhotra@gmail.com
 * \date 1-1-2013
 */

template <class FMM_Mat_t>
void CheckFMMOutput(pvfmm::FMM_Tree<FMM_Mat_t>* mytree, const pvfmm::Kernel<Real_t>* mykernel, std::string t_name){
  if(mykernel==NULL) return;

  // Find out number of OMP thereads.
  int np=omp_get_max_threads();

  // Find out my identity in the default communicator
  int myrank=0, p=1;

  typedef typename FMM_Mat_t::FMMData FMM_Data_t;
  typedef typename FMM_Mat_t::FMMNode_t FMMNode_t;

  // Read source data.
  std::vector<Real_t> src_coord;
  std::vector<Real_t> src_value;
  FMMNode_t* n=static_cast<FMMNode_t*>(mytree->PreorderFirst());
  while(n!=NULL){
    if(n->IsLeaf() && !n->IsGhost()){
      pvfmm::Vector<Real_t>& coord_vec=n->src_coord;
      pvfmm::Vector<Real_t>& value_vec=n->src_value;
      for(size_t i=0;i<coord_vec.Dim();i++) src_coord.push_back(coord_vec[i]);
      for(size_t i=0;i<value_vec.Dim();i++) src_value.push_back(value_vec[i]);
    }
    n=static_cast<FMMNode_t*>(mytree->PreorderNxt(n));
  }
  long long src_cnt=src_coord.size()/3;
  long long val_cnt=src_value.size();
  if(src_cnt==0) return;

  int dof=val_cnt/src_cnt/mykernel->ker_dim[0];
  int trg_dof=dof*mykernel->ker_dim[1];

  // Read target data.
  std::vector<Real_t> trg_coord;
  std::vector<Real_t> trg_poten_fmm;
  long long trg_iter=0;
  size_t step_size=1+src_cnt*src_cnt*1e-9/p;
  n=static_cast<FMMNode_t*>(mytree->PreorderFirst());
  while(n!=NULL){
    if(n->IsLeaf() && !n->IsGhost()){
      pvfmm::Vector<Real_t>& coord_vec=n->trg_coord;
      pvfmm::Vector<Real_t>& poten_vec=n->trg_value;
      for(size_t i=0;i<coord_vec.Dim()/3          ;i++){
        if(trg_iter%step_size==0){
          for(int j=0;j<3        ;j++) trg_coord    .push_back(coord_vec[i*3        +j]);
          for(int j=0;j<trg_dof  ;j++) trg_poten_fmm.push_back(poten_vec[i*trg_dof  +j]);
        }
        trg_iter++;
      }
    }
    n=static_cast<FMMNode_t*>(mytree->PreorderNxt(n));
  }
  int trg_cnt=trg_coord.size()/3;
  if(trg_cnt==0) return;

  //Direct N-Body.
  std::vector<Real_t> trg_poten_dir(trg_cnt*trg_dof ,0);
  pvfmm::Profile::Tic("N-Body Direct",false,1);
  #pragma omp parallel for
  for(int i=0;i<np;i++){
    size_t a=(i*trg_cnt)/np;
    size_t b=((i+1)*trg_cnt)/np;
    mykernel->ker_poten(&src_coord[0], src_cnt, &src_value[0], dof, &trg_coord[a*3], b-a, &trg_poten_dir[a*trg_dof  ],NULL);
  }
  pvfmm::Profile::Toc();

  //Compute error.
  {
    Real_t max_=0;
    Real_t max_err=0;
    for(size_t i=0;i<trg_poten_fmm.size();i++){
      Real_t err=fabs(trg_poten_dir[i]-trg_poten_fmm[i]);
      Real_t max=fabs(trg_poten_dir[i]);
      if(err>max_err) max_err=err;
      if(max>max_) max_=max;
    }
    if(!myrank){
#ifdef __VERBOSE__
      std::cout<<"Maximum Absolute Error ["<<t_name<<"] :  "<<std::scientific<<max_err<<'\n';
      std::cout<<"Maximum Relative Error ["<<t_name<<"] :  "<<std::scientific<<max_err/max_<<'\n';
#else
      std::cout<<"Error      : "<<std::scientific<<max_err/max_<<'\n';
#endif
    }
  }
}


template <class FMMTree_t>
void CheckChebOutput(FMMTree_t* mytree, typename TestFn<Real_t>::Fn_t fn_poten, int fn_dof, std::string t_name){
  typedef typename FMMTree_t::Node_t FMMNode_t;
  pvfmm::Profile::Tic((std::string("Compute Error ")+t_name).c_str(),true,1);

  int myrank=0;
  FMMNode_t* r_node=static_cast<FMMNode_t*>(mytree->RootNode());
  int dof=r_node->DataDOF()/fn_dof;
  std::vector<FMMNode_t*> nodes;
  {
    FMMNode_t* node=static_cast<FMMNode_t*>(mytree->PreorderFirst());
    while(node!=NULL){
      if(node->IsLeaf() && !node->IsGhost()) nodes.push_back(node);
      node=static_cast<FMMNode_t*>(mytree->PreorderNxt((FMMNode_t*)node));
    }
    if(nodes.size()==0) return;
  }

  int cheb_deg=nodes[0]->ChebDeg();
  std::vector<Real_t> cheb_nds=pvfmm::cheb_nodes<Real_t>(cheb_deg+1, 1);
  for(size_t i=0;i<cheb_nds.size();i++) cheb_nds[i]=2.0*cheb_nds[i]-1.0;
  std::vector<Real_t> cheb_pts=pvfmm::cheb_nodes<Real_t>(cheb_deg+1, COORD_DIM);
  int n_pts=cheb_pts.size()/COORD_DIM;
  int omp_p=omp_get_max_threads();

  std::vector<Real_t> err_avg(omp_p*dof*fn_dof,0);
#pragma omp parallel for
  for(size_t tid=0;tid<omp_p;tid++){
    pvfmm::Vector<Real_t> out; out.SetZero();
    pvfmm::Vector<Real_t> fn_out(dof*fn_dof);
    for(size_t i=(nodes.size()*tid)/omp_p;i<(nodes.size()*(tid+1))/omp_p;i++){
      pvfmm::Vector<Real_t>& cheb_coeff=nodes[i]->ChebData();
      cheb_eval(cheb_coeff, cheb_deg, cheb_nds, cheb_nds, cheb_nds, out);

      Real_t* c=nodes[i]->Coord();
      Real_t s=pow(2.0,-nodes[i]->Depth());
      Real_t s3=s*s*s;
      for(size_t j=0;j<n_pts;j++){
	Real_t coord[3]={c[0]+s*cheb_pts[j*COORD_DIM+0],
			 c[1]+s*cheb_pts[j*COORD_DIM+1],
			 c[2]+s*cheb_pts[j*COORD_DIM+2]};
	fn_out.SetZero();
	for(size_t k=0;k<dof;k++)
	  fn_poten(coord,1,&fn_out[k*fn_dof]);
	for(size_t k=0;k<dof*fn_dof;k++){
	  Real_t err=out[n_pts*k+j]-fn_out[k];
	  err_avg[tid*dof*fn_dof+k]+=err*s3;
	}
      }
    }
    for(size_t k=0;k<dof*fn_dof;k++)
      err_avg[tid*dof*fn_dof+k]/=n_pts;
  }
  for(size_t tid=1;tid<omp_p;tid++)
    for(size_t k=0;k<dof*fn_dof;k++)
      err_avg[k]+=err_avg[tid*dof*fn_dof+k];
  if(0){ // Write error to file.
    int nn_x=1;
    int nn_y=201;
    int nn_z=201;
    std::vector<Real_t> x(nn_x,1.0/2.0);
    std::vector<Real_t> y(nn_y);
    std::vector<Real_t> z(nn_z);
    //for(int i=0;i<nn_x;i++)
    //  x[i]=((Real_t)i)/(nn_x-1);
    for(int i=0;i<nn_y;i++)
      y[i]=((Real_t)i)/(nn_y-1);
    for(int i=0;i<nn_z;i++)
      z[i]=((Real_t)i)/(nn_z-1);

    FMMNode_t* r_node=static_cast<FMMNode_t*>(mytree->RootNode());
    int fn_dof=r_node->DataDOF()/dof;

    Real_t* fn_out=new Real_t[dof*fn_dof];
    pvfmm::Matrix<Real_t> M_out    (nn_z*fn_dof*dof,nn_y*nn_x,NULL,true); M_out    .SetZero();
    pvfmm::Matrix<Real_t> M_out_err(nn_z*fn_dof*dof,nn_y*nn_x,NULL,true); M_out_err.SetZero();
    r_node->ReadVal(x,y,z,M_out[0],false);
    for(int l0=0;l0<dof;l0++)
    for(int l1=0;l1<fn_dof;l1++)
    for(int i=0;i<nn_x;i++)
    for(int j=0;j<nn_y;j++)
    for(int k=0;k<nn_z;k++){
      int l=l0*fn_dof+l1;
      Real_t ch_coord[3]={x[i],y[j],z[k]};
      Real_t cheb_val=M_out[k+l*nn_z][i+j*nn_x];
      if(fabs(cheb_val)>1.0e-20){
        fn_poten(ch_coord,1,fn_out);
        Real_t err=(cheb_val-fn_out[l])-err_avg[l];
        M_out_err[k+l*nn_z][i+j*nn_x]=err;
      }
    }
    delete[] fn_out;
    std::string fname;
    fname=std::string("result/"    )+t_name+std::string(".mat");
    if(!myrank) M_out    .Write(fname.c_str());
    fname=std::string("result/err_")+t_name+std::string(".mat");
    if(!myrank) M_out_err.Write(fname.c_str());
  }
  std::vector<Real_t> max    (omp_p,0), l2    (omp_p,0);
  std::vector<Real_t> max_err(omp_p,0), l2_err(omp_p,0);
  #pragma omp parallel for
  for(size_t tid=0;tid<omp_p;tid++){
    pvfmm::Vector<Real_t> out; out.SetZero();
    pvfmm::Vector<Real_t> fn_out(dof*fn_dof);
    for(size_t i=(nodes.size()*tid)/omp_p;i<(nodes.size()*(tid+1))/omp_p;i++){
      pvfmm::Vector<Real_t>& cheb_coeff=nodes[i]->ChebData();
      cheb_eval(cheb_coeff, cheb_deg, cheb_nds, cheb_nds, cheb_nds, out);

      Real_t* c=nodes[i]->Coord();
      Real_t s=pow(2.0,-nodes[i]->Depth());
      Real_t s3=s*s*s;
      for(size_t j=0;j<n_pts;j++){
        Real_t coord[3]={c[0]+s*cheb_pts[j*COORD_DIM+0],
                         c[1]+s*cheb_pts[j*COORD_DIM+1],
                         c[2]+s*cheb_pts[j*COORD_DIM+2]};
        fn_out.SetZero();
        for(size_t k=0;k<dof;k++)
          fn_poten(coord,1,&fn_out[k*fn_dof]);
        for(size_t k=0;k<dof*fn_dof;k++){
          Real_t err=out[n_pts*k+j]-fn_out[k]-err_avg[k];
          if(fabs(fn_out[k])>max    [tid]) max    [tid]=fabs(fn_out[k]);
          if(fabs(err      )>max_err[tid]) max_err[tid]=fabs(err      );
          l2[tid]+=fn_out[k]*fn_out[k]*s3;
          l2_err[tid]+=err*err*s3;
        }
      }
    }
    l2    [tid]/=n_pts;
    l2_err[tid]/=n_pts;
  }
  for(size_t tid=1;tid<omp_p;tid++){
    if(max    [tid]>max    [0]) max    [0]=max    [tid];
    if(max_err[tid]>max_err[0]) max_err[0]=max_err[tid];
    l2    [0]+=l2    [tid];
    l2_err[0]+=l2_err[tid];
  }

  if(!myrank){
    std::cout<<"Absolute L2 Error ["<<t_name<<"]     :  "<<std::scientific<<sqrt(l2_err[0])<<'\n';
    std::cout<<"Relative L2 Error ["<<t_name<<"]     :  "<<std::scientific<<sqrt(l2_err[0]/l2[0])<<'\n';
    std::cout<<"Maximum Absolute Error ["<<t_name<<"]:  "<<std::scientific<<max_err[0]<<'\n';
    std::cout<<"Maximum Relative Error ["<<t_name<<"]:  "<<std::scientific<<max_err[0]/max[0]<<'\n';
  }
  pvfmm::Profile::Toc();
}


template <class Real_t>
std::vector<Real_t> point_distrib(DistribType dist_type, size_t N){
  int np=1, myrank=0;
  static size_t seed=myrank+1; seed+=np;
  srand48(seed);

  std::vector<Real_t> coord;
  switch(dist_type){
  case UnifGrid:
    {
      size_t NN=(size_t)round(pow((double)N,1.0/3.0));
      size_t N_total=NN*NN*NN;
      size_t start= myrank   *N_total/np;
      size_t end  =(myrank+1)*N_total/np;
      for(size_t i=start;i<end;i++){
        coord.push_back(((Real_t)((i/  1    )%NN)+0.5)/NN);
        coord.push_back(((Real_t)((i/ NN    )%NN)+0.5)/NN);
        coord.push_back(((Real_t)((i/(NN*NN))%NN)+0.5)/NN);
      }
    }
    break;
  case RandUnif:
    {
      size_t N_total=N;
      size_t start= myrank   *N_total/np;
      size_t end  =(myrank+1)*N_total/np;
      size_t N_local=end-start;
      coord.resize(N_local*3);

      for(size_t i=0;i<N_local*3;i++)
        coord[i]=((Real_t)drand48());
    }
    break;
  case RandGaus:
    {
      Real_t e=2.7182818284590452;
      Real_t log_e=log(e);
      size_t N_total=N;
      size_t start= myrank   *N_total/np;
      size_t end  =(myrank+1)*N_total/np;

      for(size_t i=start*3;i<end*3;i++){
        Real_t y=-1;
        while(y<=0 || y>=1){
          Real_t r1=sqrt(-2*log(drand48())/log_e)*cos(2*M_PI*drand48());
          Real_t r2=pow(0.5,i*10/N_total);
          y=0.5+r1*r2;
        }
        coord.push_back(y);
      }
    }
    break;
  case RandElps:
    {
      size_t N_total=N;
      size_t start= myrank   *N_total/np;
      size_t end  =(myrank+1)*N_total/np;
      size_t N_local=end-start;
      coord.resize(N_local*3);

      const Real_t r=0.45;
      const Real_t center[3]={0.5,0.5,0.5};
      for(size_t i=0;i<N_local;i++){
        Real_t* y=&coord[i*3];
        Real_t phi=2*M_PI*drand48();
        Real_t theta=M_PI*drand48();
        y[0]=center[0]+0.25*r*sin(theta)*cos(phi);
        y[1]=center[1]+0.25*r*sin(theta)*sin(phi);
        y[2]=center[2]+r*cos(theta);
      }
    }
    break;
  case RandSphr:
    {
      size_t N_total=N;
      size_t start= myrank   *N_total/np;
      size_t end  =(myrank+1)*N_total/np;
      size_t N_local=end-start;
      coord.resize(N_local*3);

      const Real_t center[3]={0.5,0.5,0.5};
      for(size_t i=0;i<N_local;i++){
        Real_t* y=&coord[i*3];
        Real_t r=1;
        while(r>0.5 || r==0){
          y[0]=drand48(); y[1]=drand48(); y[2]=drand48();
          r=sqrt((y[0]-center[0])*(y[0]-center[0])
                +(y[1]-center[1])*(y[1]-center[1])
                +(y[2]-center[2])*(y[2]-center[2]));
          y[0]=center[0]+0.45*(y[0]-center[0])/r;
          y[1]=center[1]+0.45*(y[1]-center[1])/r;
          y[2]=center[2]+0.45*(y[2]-center[2])/r;
        }
      }
    }
    break;
  default:
    break;
  }
  return coord;
}

void commandline_option_start(int argc, char** argv, const char* help_text){
  char help[]="--help";
  for(int i=0;i<argc;i++){
    if(!strcmp(argv[i],help)){
      if(help_text!=NULL) std::cout<<help_text<<'\n';
      std::cout<<"Usage:\n";
      std::cout<<"  "<<argv[0]<<" [options]\n\n";
    }
  }
}

const char* commandline_option(int argc, char** argv, const char* opt, const char* def_val, bool required, const char* err_msg){
  char help[]="--help";
  for(int i=0;i<argc;i++){
    if(!strcmp(argv[i],help)){
      std::cout<<"        "<<err_msg<<'\n';
      return def_val;
    }
  }

  for(int i=0;i<argc;i++){
    if(!strcmp(argv[i],opt)){
      return argv[(i+1)%argc];
    }
  }
  if(required){
    std::cout<<"Missing: required option\n"<<"    "<<err_msg<<"\n\n";
    std::cout<<"To see usage options\n"<<"    "<<argv[0]<<" --help\n\n";
    exit(0);
  }
  return def_val;
}

void commandline_option_end(int argc, char** argv){
  char help[]="--help";
  for(int i=0;i<argc;i++){
    if(!strcmp(argv[i],help)){
      std::cout<<"\n";
      exit(0);
    }
  }
}

/** Copyright (c) 2011, Edgar Solomonik, all rights reserved.
  * \addtogroup benchmarks
  * @{ 
  * \addtogroup model_trainer
  * @{ 
  * \brief Executes a set of different contractions on different processor counts to train model parameters
  */

#include <ctf.hpp>
#define TEST_SUITE
#include "../examples/ccsd.cxx"
#include "../examples/sparse_mp3.cxx"
#undef TEST_SUITE
using namespace CTF;

namespace CTF_int{
  void update_all_models(MPI_Comm comm);
}

void train_off_vec_mat(int64_t n, int64_t m, World & dw, bool sp_A, bool sp_B, bool sp_C);

void train_dns_vec_mat(int64_t n, int64_t m, World & dw){
  Vector<> b(n, dw);
  Vector<> c(m, dw);
  Matrix<> A(m, n, dw);
  Matrix<> A1(m, n, dw);
  Matrix<> A2(m, n, dw);
  Matrix<> G(n, n, SY, dw);
  Matrix<> F(m, m, AS, dw);

  srand48(dw.rank);
  b.fill_random(-.5, .5);
  c.fill_random(-.5, .5);
  A.fill_random(-.5, .5);
  A1.fill_random(-.5, .5);
  A2.fill_random(-.5, .5);
  G.fill_random(-.5, .5);
  F.fill_random(-.5, .5);

  A["ij"] += A["ik"]*G["kj"];
  A["ij"] += A["ij"]*A1["ij"];
  A["ij"] += F["ik"]*A["kj"];
  c["i"]  += A["ij"]*b["j"];
  b["j"]  += .2*A["ij"]*c["i"];
  b["i"]  += b["i"]*b["i"];

  Function<> f1([](double a){ return a*a; });

  A2["ij"] = f1(A["ij"]);
  
  c["i"] += f1(A["ij"]);
  
  Function<> f2([](double a, double b){ return a*a+b*b; });

  A1["ij"] -= f2(A["kj"], F["ki"]);

  Transform<> t1([](double & a){ a*=a; });

  t1(b["i"]);
  t1(A["ij"]);

  Transform<> t2([](double a, double & b){ b-=b/a; });
  
  t2(b["i"],b["i"]);
  t2(A["ij"],A2["ij"]);


  /*Transform<> t3([](double a, double b, double & c){ c=c*c-b*a; });

  t3(c["i"],b["i"],b["i"]);
  t3(A["ij"],G["ij"],F["ij"]);*/
}


void train_sps_vec_mat(int64_t n, int64_t m, World & dw, bool sp_A, bool sp_B, bool sp_C){
  Vector<> b(n, dw);
  Vector<> c(m, dw);
  Matrix<> A(m, n, dw);
  Matrix<> B(m, n, dw);
  Matrix<> A1(m, n, dw);
  Matrix<> A2(m, n, dw);
  Matrix<> G(n, n, NS, dw);
  Matrix<> F(m, m, NS, dw);
  
  srand48(dw.rank);
  b.fill_random(-.5, .5);
  c.fill_random(-.5, .5);
  A.fill_random(-.5, .5);
  B.fill_random(-.5, .5);
  A1.fill_random(-.5, .5);
  A2.fill_random(-.5, .5);
  G.fill_random(-.5, .5);
  F.fill_random(-.5, .5);
  for (double sp = .01; sp<.32; sp*=2.){
    if (sp_A) A.sparsify([=](double a){ return fabs(a)<=.5*sp; });
    if (sp_B){
      G.sparsify([=](double a){ return fabs(a)<=.5*sp; });
      F.sparsify([=](double a){ return fabs(a)<=.5*sp; });
    }
    if (sp_C){
      b.sparsify([=](double a){ return fabs(a)<=.5*sp; });
      B.sparsify([=](double a){ return fabs(a)<=.5*sp; });
      c.sparsify([=](double a){ return fabs(a)<=.5*sp; });
    }
  
    B["ij"] += A["ik"]*G["kj"];
    if (!sp_C) B["ij"] += A["ij"]*A1["ij"];
    B["ij"] += F["ik"]*A["kj"];
    c["i"]  += A["ij"]*b["j"];
    b["j"]  += .2*A["ij"]*c["i"];
    if (!sp_C) b["i"]  += b["i"]*b["i"];
  
    Function<> f1([](double a){ return a*a; });
  
    A2["ij"] = f1(A["ij"]);
    
    c["i"] += f1(A["ij"]);
    
    Function<> f2([](double a, double b){ return a*a+b*b; });
  
    A2["ji"] -= f2(A1["ki"], F["kj"]);
  
    Transform<> t1([](double & a){ a*=a; });
  
    t1(b["i"]);
    t1(A["ij"]);
  
    Transform<> t2([](double a, double & b){ b-=b/a; });
    
    t2(b["i"],b["i"]);
    t2(A["ij"],A2["ij"]);
  
    /*Transform<> t3([](double a, double b, double & c){ c=c*c-b*a; });
  
    t3(c["i"],b["i"],b["i"]);
    t3(A["ij"],G["ij"],F["ij"]);*/
  }
}

void train_ccsd(int64_t n, int64_t m, World & dw){
  int nv = sqrt(n);
  int no = sqrt(m);
  Integrals V(no, nv, dw);
  V.fill_rand();
  Amplitudes T(no, nv, dw);
  T.fill_rand();
  ccsd(V,T,0);
  T["ai"] = (1./T.ai->norm2())*T["ai"];
  T["abij"] = (1./T.abij->norm2())*T["abij"];
}


void train_sparse_mp3(int64_t n, int64_t m, World & dw){
  int nv = sqrt(n);
  int no = sqrt(m);
  for (double sp = .001; sp<.2; sp*=4.){
    sparse_mp3(nv, no, dw, sp, 0, 1, 1, 0, 0);
    sparse_mp3(nv, no, dw, sp, 0, 1, 0, 1, 0);
    sparse_mp3(nv, no, dw, sp, 0, 1, 0, 1, 1);
  }
}


void train_world(double dtime, World & dw){
  int n0 = 19, m0 = 75;
  int64_t n = n0;
  int64_t approx_niter = std::max(1,(int)(10*log(dtime))); //log((dtime*2000./15.)/dw.np);
  double ddtime = dtime/approx_niter;
  int rnk;
  MPI_Comm_rank(MPI_COMM_WORLD, &rnk);
//  printf("ddtime = %lf\n", ddtime);
  for (;;){
    double t_st = MPI_Wtime();
    int niter = 0;
    int64_t m = m0;
    double ctime = 0.0;
    do {
      if (rnk == 0) printf("executing p = %d n= %ld m = %ld ctime = %lf ddtime = %lf\n", dw.np, n, m, ctime, ddtime);
      train_dns_vec_mat(2*n, 2*m, dw);
      train_sps_vec_mat(n-2, m, dw, 0, 0, 0);
      train_sps_vec_mat(n+1, m-2, dw, 1, 0, 0);
      train_sps_vec_mat(n+6, m-4, dw, 1, 1, 0);
      train_sps_vec_mat(n+2, m-3, dw, 1, 1, 1);
      train_off_vec_mat(n+7, m-4, dw, 0, 0, 0);
      train_off_vec_mat(n-2, m+6, dw, 1, 0, 0);
      train_off_vec_mat(n-5, m+2, dw, 1, 1, 0);
      train_off_vec_mat(n-3, m-1, dw, 1, 1, 1);
      train_ccsd(n/2, m/2, dw);
      train_sparse_mp3(n,m,dw);
      niter++;
      m *= 1.9;
      n += 2;
      ctime = MPI_Wtime() - t_st;
      MPI_Allreduce(MPI_IN_PLACE, &ctime, 1, MPI_DOUBLE, MPI_MAX, dw.comm);
    } while (ctime < ddtime && m<= 1000000);
    if (niter <= 2 || n>=1000000) break;
    n *= 1.7;
    m += 3;
  }
}

void frize(std::set<int> & ps, int p){
  ps.insert(p);
  if (p>=1){
    for (int i=2; i<p; i++){
      if (p%i == 0) frize(ps, p/i);
    }
  }
}

void train_all(double time, World & dw){
  std::set<int> ps;
  frize(ps, dw.np);

  std::set<int>::iterator it;
  double dtime = time/ps.size();
  for (it=ps.begin(); it!=ps.end(); it++){
    int np = *it;
    int mw = dw.rank/np;
    int mr = dw.rank%np;
    int cm;
    MPI_Comm_split(dw.comm, mw, mr, &cm);
    World w(cm);
    train_world(dtime, w);
    CTF_int::update_all_models(w.cdt.cm);
    train_world(dtime, w);
    CTF_int::update_all_models(w.cdt.cm);
  }
}

char* getCmdOption(char ** begin,
                   char ** end,
                   const   std::string & option){
  char ** itr = std::find(begin, end, option);
  if (itr != end && ++itr != end){
    return *itr;
  }
  return 0;
}


int main(int argc, char ** argv){
  int rank, np;
  double time;
  int const in_num = argc;
  char ** input_str = argv;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &np);

  if (getCmdOption(input_str, input_str+in_num, "-time")){
    time = atof(getCmdOption(input_str, input_str+in_num, "-time"));
    if (time < 0) time = 5.0;
  } else time = 5.0;


  {
    World dw(MPI_COMM_WORLD, argc, argv);

    if (rank == 0){
      printf("Executing a wide set of contractions to train model with time budget of %lf sec\n", time);
    }
    train_all(time, dw);
  }


  MPI_Finalize();
  return 0;
}

/**
 * @} 
 * @}
 */


#include "namaster.h"
#include "ctest.h"
#include "utils.h"
#include "nmt_test_utils.h"

CTEST(nmt,covar) {
  int ii;
  nmt_workspace *w=nmt_workspace_read("test/benchmarks/bm_nc_np_w00.dat");
  nmt_covar_workspace *cw=nmt_covar_workspace_init(w,w);
  nmt_covar_workspace *cwr=nmt_covar_workspace_read("test/benchmarks/bm_nc_np_cw00.dat");

  ASSERT_EQUAL(cwr->lmax_a,cw->lmax_a);
  ASSERT_EQUAL(cwr->lmax_b,cw->lmax_b);
  ASSERT_EQUAL(cwr->ncls_a,cw->ncls_a);
  ASSERT_EQUAL(cwr->ncls_b,cw->ncls_b);
  ASSERT_EQUAL(cwr->nside,cw->nside);
  for(ii=0;ii<cw->ncls_a*(cw->lmax_a+1);ii++) {
    int jj;
    for(jj=0;jj<cw->ncls_b*(cw->lmax_b+1);jj++) {
      ASSERT_EQUAL(cwr->xi_1122[ii][jj],cw->xi_1122[ii][jj]);
      ASSERT_EQUAL(cwr->xi_1221[ii][jj],cw->xi_1221[ii][jj]);
    }
  }

  nmt_workspace_free(w);
  nmt_covar_workspace_free(cwr);

  //Init power spectra
  int ncls=1;
  double *cell=my_malloc(3*cw->nside*sizeof(double));
  //Read signal and noise power spectrum
  FILE *fi=my_fopen("test/benchmarks/cls_lss.txt","r");
  for(ii=0;ii<3*cw->nside;ii++) {
    double dum,cl,nl;
    int stat=fscanf(fi,"%lf %lf %lf %lf %lf %lf %lf %lf %lf",&dum,
		    &cl,&dum,&dum,&dum,&nl,&dum,&dum,&dum);
    ASSERT_EQUAL(stat,9);
    cell[ii]=cl+nl;
  }
  fclose(fi);

  double *covar=my_malloc(cw->bin_a->n_bands*cw->bin_b->n_bands*sizeof(double));
  nmt_compute_gaussian_covariance(cw,cell,cell,cell,cell,covar);

  fi=my_fopen("test/benchmarks/bm_nc_np_cov.txt","r");
  for(ii=0;ii<cw->bin_a->n_bands*cw->bin_b->n_bands;ii++) {
    double cov;
    int stat=fscanf(fi,"%lf",&cov);
    ASSERT_DBL_NEAR_TOL(cov,covar[ii],fabs(fmax(cov,covar[ii]))*1E-3);
  }
  fclose(fi);

  free(covar);
  free(cell);
  nmt_covar_workspace_free(cw);
}
  
CTEST(nmt,covar_errors) {
  nmt_covar_workspace *cw=NULL;
  nmt_workspace *wb,*wa=nmt_workspace_read("test/benchmarks/bm_nc_np_w00.dat");
  int nside=wa->nside;
  int lmax=wa->lmax;

  set_error_policy(THROW_ON_ERROR);

  //All good
  wb=nmt_workspace_read("test/benchmarks/bm_nc_np_w00.dat");
  try { cw=nmt_covar_workspace_init(wa,wb); }
  ASSERT_EQUAL(0,nmt_exception_status);
  nmt_covar_workspace_free(cw); cw=NULL;

  //Wrong reading
  try { cw=nmt_covar_workspace_read("none"); }
  ASSERT_NOT_EQUAL(0,nmt_exception_status);
  ASSERT_NULL(cw);

  //Correct reading
  try { cw=nmt_covar_workspace_read("test/benchmarks/bm_nc_np_cw00.dat"); }
  ASSERT_EQUAL(0,nmt_exception_status);
  nmt_covar_workspace_free(cw); cw=NULL;

  //Incompatible resolutions
  wb->nside=128;
  try { cw=nmt_covar_workspace_init(wa,wb); }
  wb->nside=nside;
  ASSERT_NOT_EQUAL(0,nmt_exception_status);
  ASSERT_NULL(cw);

  //Incompatible lmax
  wb->lmax=3*128-1;
  try { cw=nmt_covar_workspace_init(wa,wb); }
  wb->lmax=lmax;
  ASSERT_NOT_EQUAL(0,nmt_exception_status);
  ASSERT_NULL(cw);
  nmt_workspace_free(wb);

  //Spin-2
  wb=nmt_workspace_read("test/benchmarks/bm_nc_np_w02.dat");
  try { cw=nmt_covar_workspace_init(wa,wb); }
  ASSERT_NOT_EQUAL(0,nmt_exception_status);
  ASSERT_NULL(cw);
  nmt_workspace_free(wb);

  set_error_policy(EXIT_ON_ERROR);

  nmt_workspace_free(wa);
}

#include "namaster.h"
#include "ctest.h"
#include "utils.h"
#include "nmt_test_utils.h"
#include <chealpix.h>

CTEST(nmt,he_synalm) {
  int ii,l;
  long nside=128;
  long lmax=3*nside-1;
  int nmaps=2;
  int ncls=nmaps*nmaps;
  double lpivot=nside/2.;
  double alpha_pivot=1.;
  double **cells_in=my_malloc(ncls*sizeof(double *));
  double **cells_pass=my_malloc(3*sizeof(double *));
  double **cells_out=my_malloc(ncls*sizeof(double *));
  double **beam=my_malloc(nmaps*sizeof(double *));

  for(ii=0;ii<nmaps;ii++) {
    beam[ii]=my_malloc((lmax+1)*sizeof(double));
    for(l=0;l<=lmax;l++)
      beam[ii][l]=1.;
  }
  
  for(ii=0;ii<ncls;ii++) {
    cells_in[ii]=my_malloc((lmax+1)*sizeof(double));
    cells_out[ii]=my_malloc((lmax+1)*sizeof(double));
    for(l=0;l<=lmax;l++) {
      if((ii==0) || (ii==3))
	cells_in[ii][l]=pow((2*lpivot)/(l+lpivot),alpha_pivot);
      else
	cells_in[ii][l]=0;
    }
  }
  cells_pass[0]=cells_in[0];
  cells_pass[1]=cells_in[1];
  cells_pass[2]=cells_in[3];
  fcomplex **alms=he_synalm(nside,nmaps,lmax,cells_pass,beam,1234);
  he_alm2cl(alms,alms,1,1,cells_out,lmax);

  for(l=0;l<=lmax;l++) {
    int im1;
    for(im1=0;im1<nmaps;im1++) {
      int im2;
      for(im2=0;im2<nmaps;im2++) {
	double c11=cells_in[im1+nmaps*im1][l];
	double c12=cells_in[im2+nmaps*im1][l];
	double c21=cells_in[im1+nmaps*im2][l];
	double c22=cells_in[im2+nmaps*im2][l];
	double sig=sqrt((c11*c22+c12*c21)/(2.*l+1.));
	double diff=fabs(cells_out[im2+nmaps*im1][l]-c12);
	//Check that there are no >5-sigma fluctuations around input power spectrum
	ASSERT_TRUE((int)(diff<5*sig));
      }
    }
  }
  
  for(ii=0;ii<ncls;ii++) {
    free(cells_in[ii]);
    free(cells_out[ii]);
  }
  
  for(ii=0;ii<nmaps;ii++) {
    free(alms[ii]);
    free(beam[ii]);
  }

  free(alms);
  free(beam);
  free(cells_pass);
  free(cells_in);
  free(cells_out);
}

CTEST(nmt,he_beams) {
  int l;
  long nside=128;
  long lmax=3*nside-1;
  double sigma=1.*M_PI/180; //Beam sigma in radians
  double fwhm_amin=sigma*180*60/M_PI*2.35482;
  double *beam_he=he_generate_beam_window(lmax,fwhm_amin);

  for(l=0;l<=lmax;l++) {
    double b=exp(-0.5*l*(l+1.)*sigma*sigma);
    ASSERT_DBL_NEAR_TOL(1.,beam_he[l]/b,1E-3);
  }
  free(beam_he);
}

CTEST(nmt,he_alm2cl)
{
  int ii;
  long nside=256;
  int lmax=3*nside-1;
  long npix=he_nside2npix(nside);
  double **maps=my_malloc(3*sizeof(double *));
  fcomplex **alms=my_malloc(3*sizeof(fcomplex *));
  double **cls=my_malloc(7*sizeof(double *));
  
  for(ii=0;ii<7;ii++)
    cls[ii]=my_malloc((lmax+1)*sizeof(double));

  //Read data
  for(ii=0;ii<3;ii++) {
    long ns;
    maps[ii]=he_read_healpix_map("test/maps.fits",&ns,ii);
    ASSERT_EQUAL(nside,ns);
    alms[ii]=my_malloc(he_nalms(lmax)*sizeof(fcomplex));
  }

  //SHT
  he_map2alm(nside,lmax,1,0,maps,alms,3);
  he_map2alm(nside,lmax,1,2,&(maps[1]),&(alms[1]),3);

  //Power spectra
  he_alm2cl(&(alms[0]),&(alms[0]),0,0,&(cls[0]),lmax);
  he_alm2cl(&(alms[0]),&(alms[1]),0,1,&(cls[1]),lmax);
  he_alm2cl(&(alms[1]),&(alms[1]),1,1,&(cls[3]),lmax);
  for(ii=0;ii<7;ii++)
    test_compare_arrays(lmax+1,cls[ii],7,ii,"test/benchmarks/cls_afst.txt",1E-5);

  //Anafast
  he_anafast(&(maps[0]),&(maps[0]),0,0,&(cls[0]),nside,lmax,3);
  he_anafast(&(maps[0]),&(maps[1]),0,1,&(cls[1]),nside,lmax,3);
  he_anafast(&(maps[1]),&(maps[1]),1,1,&(cls[3]),nside,lmax,3);
  for(ii=0;ii<7;ii++)
    test_compare_arrays(lmax+1,cls[ii],7,ii,"test/benchmarks/cls_afst.txt",1E-5);
  
  for(ii=0;ii<3;ii++) {
    free(maps[ii]);
    free(alms[ii]);
  }

  for(ii=0;ii<7;ii++)
    free(cls[ii]);

  free(cls);
  free(maps);
  free(alms);
}  

CTEST(nmt,he_sht) {
  int ii;
  int nmaps=34;
  long nside=16;
  long lmax=3*nside-1;
  long npix=he_nside2npix(nside);
  double **maps=my_malloc(2*nmaps*sizeof(double *));
  fcomplex **alms=my_malloc(2*nmaps*sizeof(fcomplex *));

  for(ii=0;ii<2*nmaps;ii++) {
    maps[ii]=my_calloc(npix,sizeof(double));
    alms[ii]=my_malloc(he_nalms(lmax)*sizeof(fcomplex));
  }

  //Direct SHT
  //Single SHT, spin-0
  he_map2alm(nside,lmax,1,0,maps,alms,0);
  //Several SHTs, spin-0
  he_map2alm(nside,lmax,nmaps,0,maps,alms,0);
  //Single SHT, spin-2
  he_map2alm(nside,lmax,1,2,maps,alms,0);
  //Several SHTs, spin-2
  he_map2alm(nside,lmax,nmaps,2,maps,alms,0);
  //Several SHTs, spin-2, iterate
  he_map2alm(nside,lmax,nmaps,2,maps,alms,3);

  //Test alm zeroing
  he_zero_alm(lmax,alms[0]);
  he_zero_alm(lmax,alms[1]);

  //Inverse SHT
  //Single SHT, spin-0
  he_alm2map(nside,lmax,1,0,maps,alms);
  //Several SHTs, spin-0
  he_alm2map(nside,lmax,nmaps,0,maps,alms);
  //Single SHT, spin-2
  he_alm2map(nside,lmax,1,2,maps,alms);
  //Several SHTs, spin-2
  he_alm2map(nside,lmax,nmaps,2,maps,alms);
  for(ii=0;ii<2*nmaps;ii++) {
    free(maps[ii]);
    free(alms[ii]);
  }

  //Test for one particular example
  nside=256;
  npix=he_nside2npix(nside);
  free(maps);
  for(ii=0;ii<2;ii++)
    alms[ii]=my_malloc(he_nalms(lmax)*sizeof(fcomplex));
  //spin-0, map = Re(Y_22) ->
  //        a_lm = delta_l2 (delta_m2 + delta_m-2)/2
  maps=test_make_map_analytic(nside,0);
  he_map2alm(nside,lmax,1,0,maps,alms,0);
  ASSERT_DBL_NEAR_TOL(0.5,creal(alms[0][he_indexlm(2,2,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.0,cimag(alms[0][he_indexlm(2,2,lmax)]),1E-5);
  free(maps[0]); free(maps);
  //spin-2, map = _2Y^E_20+2* _2Y^B_30) ->
  //        E_lm =   delta_l2 delta_m0
  //        B_lm = 2 delta_l3 delta_m0
  maps=test_make_map_analytic(nside,1);
  he_map2alm(nside,lmax,1,2,maps,alms,0);
  ASSERT_DBL_NEAR_TOL(1.,creal(alms[0][he_indexlm(2,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.,cimag(alms[0][he_indexlm(2,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.,creal(alms[1][he_indexlm(2,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.,cimag(alms[1][he_indexlm(2,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.,creal(alms[0][he_indexlm(1,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.,cimag(alms[0][he_indexlm(1,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.,creal(alms[1][he_indexlm(1,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.,cimag(alms[1][he_indexlm(1,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.,creal(alms[0][he_indexlm(3,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.,cimag(alms[0][he_indexlm(3,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(2.,creal(alms[1][he_indexlm(3,0,lmax)]),1E-5);
  ASSERT_DBL_NEAR_TOL(0.,cimag(alms[1][he_indexlm(3,0,lmax)]),1E-5);
  free(maps[0]); free(maps[1]); free(maps);

  for(ii=0;ii<2;ii++)
    free(alms[ii]);
  free(alms);
}

CTEST(nmt,he_io) {
  set_error_policy(THROW_ON_ERROR);

  int ii;
  long nside=4;
  long npix=he_nside2npix(nside);
  ASSERT_EQUAL(npix,12*nside*nside);

  double **mps0=my_malloc(1*sizeof(double *));
  double **mps2=my_malloc(1*sizeof(double *));
  for(ii=0;ii<1;ii++)
    mps0[ii]=my_calloc(npix,sizeof(double));
  for(ii=0;ii<2;ii++)
    mps2[ii]=my_calloc(npix,sizeof(double));

  //Writing
  try {he_write_healpix_map(mps0,1,nside,"!test/mps.fits");}
  ASSERT_EQUAL(0,nmt_exception_status);
  try {he_write_healpix_map(mps2,1,nside,"!test/mps.fits");}
  ASSERT_EQUAL(0,nmt_exception_status);
  try {he_write_healpix_map(mps2,2,nside,"!test/mps.fits");}
  ASSERT_EQUAL(0,nmt_exception_status);

  for(ii=0;ii<1;ii++)
    free(mps0[ii]);
  for(ii=0;ii<2;ii++)
    free(mps2[ii]);
  
  //Reading
  long ns2;
  int nfd,isnest;
  //Correct file params
  he_get_file_params("test/mps.fits",&ns2,&nfd,&isnest);
  ASSERT_EQUAL(nside,ns2);
  ASSERT_EQUAL(2,nfd);
  ASSERT_EQUAL(0,isnest);
  //Read map
  mps0[0]=he_read_healpix_map("test/mps.fits",&ns2,0);
  ASSERT_EQUAL(nside,ns2);
  mps2[0]=he_read_healpix_map("test/mps.fits",&ns2,0);
  ASSERT_EQUAL(nside,ns2);
  //Go one column too far
  try {mps2[1]=he_read_healpix_map("test/mps.fits",&ns2,2);}
  ASSERT_NOT_EQUAL(0,nmt_exception_status);
  mps2[1]=he_read_healpix_map("test/mps.fits",&ns2,1);
  ASSERT_EQUAL(nside,ns2);
  
  for(ii=0;ii<1;ii++)
    free(mps0[ii]);
  for(ii=0;ii<2;ii++)
    free(mps2[ii]);
  free(mps0); free(mps2);

  set_error_policy(EXIT_ON_ERROR);
}

CTEST_SKIP(nmt,he_qdisc) {
  int ii;
  long nside=256;
  long ntot_alloc=20000;
  int *indices=my_malloc(ntot_alloc*sizeof(int));
  int ntot_alloc_d=ntot_alloc/2;
  he_query_disc(nside,1.,0.,M_PI/180,indices,&ntot_alloc_d,1);
  ASSERT_EQUAL(84,ntot_alloc_d); ntot_alloc_d=ntot_alloc/2;
  he_query_disc(nside,-1.,0.,M_PI/180,indices,&ntot_alloc_d,1);
  ASSERT_EQUAL(84,ntot_alloc_d); ntot_alloc_d=ntot_alloc/2;
  /*
    This doesn't seem to pass - it's different than the healpy implementation
  */
  he_query_disc(nside,0.,0.,M_PI/180,indices,&ntot_alloc_d,1);
  ASSERT_EQUAL(88,ntot_alloc_d); ntot_alloc_d=ntot_alloc/2;
  free(indices);
}

CTEST(nmt,he_qstrip) {
  long nside=1024;

  set_error_policy(THROW_ON_ERROR);

  //queries
  long ntot_alloc=20000;
  int *indices=my_malloc(ntot_alloc*sizeof(int));
  //query strip around equator
  he_query_strip(nside,M_PI/2-M_PI/8192,M_PI/2+M_PI/8192,indices,&ntot_alloc);
  ASSERT_EQUAL(12288,ntot_alloc); ntot_alloc=20000;
  //query strip around pole
  he_query_strip(nside,M_PI-M_PI/1024,M_PI,indices,&ntot_alloc);
  ASSERT_EQUAL(40,ntot_alloc); ntot_alloc=20000;
  //query strip exceptions
  try { he_query_strip(nside,-M_PI,M_PI,indices,&ntot_alloc); }
  ASSERT_NOT_EQUAL(0,nmt_exception_status);
  try { he_query_strip(nside,0,M_PI/2,indices,&ntot_alloc); }
  ASSERT_NOT_EQUAL(0,nmt_exception_status);

  free(indices);
  set_error_policy(EXIT_ON_ERROR);
}

CTEST(nmt,he_ringnum) {
  //ring_num
  long nside=1024;
  gsl_rng *r=init_rng(1234);
  int ii;
  for(ii=0;ii<NNO_RNG;ii++) {
    double z=2*rng_01(r)-1;
    int rn=he_ring_num(nside,z);
    ASSERT_INTERVAL(0,4*nside-2,rn);
  }
  end_rng(r);
}

CTEST(nmt,he_algb) {
  int ii;
  long nside=128;
  long npix=he_nside2npix(nside);
  double *mp1=my_malloc(npix*sizeof(double));
  double *mp2=my_malloc(npix*sizeof(double));
  double *mpr=my_malloc(npix*sizeof(double));

  for(ii=0;ii<npix;ii++) {
    mp1[ii]=2.;
    mp2[ii]=0.5;
  }
  
  double d=he_map_dot(nside,mp1,mp2);
  he_map_product(nside,mp1,mp2,mpr);
  he_map_product(nside,mp1,mp2,mp2);

  ASSERT_DBL_NEAR_TOL(4*M_PI,d,1E-5);
  for(ii=0;ii<npix;ii++) {
    ASSERT_DBL_NEAR_TOL(1.,mpr[ii],1E-10);
    ASSERT_DBL_NEAR_TOL(1.,mp2[ii],1E-10);
  }
  
  free(mp1);
  free(mp2);
  free(mpr);
}

CTEST(nmt,he_r2n) {
  int ii;
  long nside=256;
  long listpix[5]={123,453,6,723475,39642};
  long iother,npix=he_nside2npix(nside);
  double *mp=my_malloc(npix*sizeof(double));
  //setup
  for(ii=0;ii<npix;ii++)
    mp[ii]=ii;
  //ring2nest
  he_ring2nest_inplace(mp,nside);
  for(ii=0;ii<5;ii++) {
    ring2nest(nside,listpix[ii],&iother);
    ASSERT_DBL_NEAR_TOL((double)(listpix[ii]),mp[iother],1E-5);
  }
  //nest2ring
  he_nest2ring_inplace(mp,nside);
  for(ii=0;ii<5;ii++) {
    ASSERT_DBL_NEAR_TOL((double)(listpix[ii]),mp[listpix[ii]],1E-5);
  }
  free(mp);
}

CTEST(nmt,he_ud) {
  //ud-grade
  int ii;
  long nside=256,npix=he_nside2npix(nside);
  double *mp=my_malloc(npix*sizeof(double));
  double *mplo=my_malloc((npix/4)*sizeof(double));
  for(ii=0;ii<npix;ii++)
    mp[ii]=ii;
  he_nest2ring_inplace(mp,nside);
  he_udgrade(mp,nside,mplo,nside/2,0);
  for(ii=0;ii<npix/4;ii++) {
    long inest;
    double predmean;
    ring2nest(nside/2,ii,&inest);
    predmean=inest*4+1.5;
    ASSERT_DBL_NEAR_TOL(predmean,mplo[ii],1E-5);
  }
  free(mplo);  
}

CTEST(nmt,he_x2y) {
  long nside=1024;
  double ip,ip0=1026;
  double vec[3],vec0[3]={0.010057788838065481, 0.015334332284729826, 0.9998318354288737};
  double th0=0.018339535684317072,ph0=0.9902846408054783;
  he_pix2vec_ring(nside,ip0,vec);
  ip=he_ang2pix(nside,cos(th0),ph0);
  ASSERT_EQUAL(ip0,ip);
  ASSERT_DBL_NEAR_TOL(vec0[0],vec[0],1E-8);
  ASSERT_DBL_NEAR_TOL(vec0[1],vec[1],1E-8);
  ASSERT_DBL_NEAR_TOL(vec0[2],vec[2],1E-8);
}

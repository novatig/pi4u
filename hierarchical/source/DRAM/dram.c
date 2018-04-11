#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "fitfun.h"


int compare (const void * a, const void * b)
{
	return (*(double*)a - *(double*)b);
}

double sign(double a)
{
	if (a > 0) return +1;
	if (a < 0) return -1;
	return 0;
}

double norm(double *a, int n)
{
	double s = 0;
	for (int i = 0; i < n; i++)
		s += pow(a[i],2.0);

	return sqrt(s);
}

void covcond(double c, double *a, double *Sig, double *Lam, int n)
{
	double e[n];

	for (int i = 0; i < n; i++)
	{
		e[i] = c + i*((1-c)/(n-1));
		e[i] = 1.0/e[i];
	}

	qsort (e, n, sizeof(double), compare);
	a[0] = a[0] + sign(a[0]) * norm(a, n);

	for (int i = 0; i < n; i++)
	{
		printf("a[%d] = %f\n", i, a[i]);		
	}

	double norma2 = pow(norm(a, n), 2);
	double z[n][n];

	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			if (i == j)
				z[i][j] = 1.0 + (-2.0/norma2)*a[i]*a[j];
			else
				z[i][j] = (-2.0/norma2)*a[i]*a[j];

	double S[n][n], L[n][n];
	double ze[n][n], zt[n][n];

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			ze[i][j] = z[i][j]*e[j];
			zt[i][j] = z[j][i];
		}
	}

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			double s = 0.0;
			for (int k = 0; k < n; k++)
				s += ze[i][k]*zt[k][j];
			S[i][j] = s;
		}
	}


	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			ze[i][j] = z[i][j]*(1.0/e[j]);
			zt[i][j] = z[j][i];
		}
	}


	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			double s = 0.0;
			for (int k = 0; k < n; k++)
				s += ze[i][k]*zt[k][j];
			L[i][j] = s;
		}
	}


	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			Sig[i*n+j] = S[i][j];
			Lam[i*n+j] = L[i][j];
		}
	}
}




double *d_mu;
double *d_Lam;



double ssfun(double *x, int n)
{
	double s = -fitfun(x, n, NULL, NULL);
	return s;
}




double priorfun(double *x, int n)
{
	return 0;
}




struct _params
{
	double *par0;		// = mu+0.1; // initial value
	double sigma2;		// = 1;
	double n0;		// = -1;
	double *lbounds;	// = -inf
	double *ubounds;	// = +inf
	char filename[256];	// = "chain.txt"

	int n;			// = 0;
} params;


void init_params()
{
	params.par0 = NULL;

	params.sigma2 = 1;
	params.n0 = -1;

	params.lbounds = NULL;
	params.ubounds = NULL;
	strcpy(params.filename, "chain.txt");

	params.n = 0;
}

struct _options
{
	int nsimu;		//  =nsimu;
	int adaptint;		// = 500;
	double *qcov;     	//= Sig.*2.4^2./npar;
	double drscale; 	// = drscale;
	double adascale;	// = adascale; // default is 2.4/sqrt(npar) ;

	int printint;		// = 2000;
	double qcovadj;		// = 1e-5;

	int verbosity;		// = 1;
} options;

void init_options()
{
	options.qcovadj = 1e-5;
	options.verbosity = 1;
}








#include "dramrun.c"








int main(int argc, char *argv[])
{
	fitfun_initialize( );
	gsl_rand_init(1);

	const int npar    = 4;     // dimension of the target
	double drscale    = 20;     // DR shrink factor
	double adascale   = 2.4/sqrt(npar); // scale for adaptation
	int nsimu         = 1e5;    // number of simulations
	int pos = 0;           // positivity?

	// with positivity, you need nsimu to be at least
	// npar ->  nsimu 
	// 10   ->  10 000
	// 20   ->  500 000
	// 100  ->  5 000 000 ?

	double c = 10;  // cond number of the target covariance 

	double *a = (double *)malloc(npar*sizeof(double));
	for (int i = 0; i < npar; i++)
		a[i] = 1;

	double Sig[npar*npar], Lam[npar*npar];
	covcond(c, a, Sig, Lam, npar);

	double *mu = (double *)malloc(npar*sizeof(double));
	for (int i = 0; i < npar; i++)
		mu[i] = 0;

	init_params();
	params.par0    = (double *)malloc(npar*sizeof(double));
	for (int i = 0; i < npar; i++)
		params.par0[i] = mu[i]+0.1;	// initial value

	params.lbounds = (double *)malloc(npar*sizeof(double));
	for (int i = 0; i < npar; i++)
		params.lbounds[i] = -1e10;	// -inf

	params.ubounds = (double *)malloc(npar*sizeof(double));
	for (int i = 0; i < npar; i++)
		params.ubounds[i] = +1e10;	// -inf

	if (pos)
	{
		for (int i = 0; i < npar; i++)
			params.lbounds[i] = 0;
	}


	d_mu = mu;
	d_Lam = Lam;

	init_options();
	options.nsimu    = nsimu;
	options.adaptint = 500;

	options.qcov = (double *)malloc(npar*npar*sizeof(double));
	for (int i = 0; i < npar; i++)
	for (int j = 0; j < npar; j++)
		options.qcov[i*npar+j] = Sig[i*npar+j]*pow(2.4,2)/npar;

	options.drscale  = drscale;
	options.adascale = adascale; // default is 2.4/sqrt(npar) ;
	options.printint = 2000;

	for (int i = 0; i < npar; i++)
		for (int j = 0; j < npar; j++)
			printf("options.qcov[%d][%d]=%f\n", i, j, options.qcov[i*npar+j]);




	dramrun(npar);




	fitfun_finalize();

	return 0;
}





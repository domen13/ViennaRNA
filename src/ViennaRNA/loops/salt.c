#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <math.h>
#include "ViennaRNA/utils/basic.h"
#include "ViennaRNA/loops/salt.h"

#ifdef __GNUC__
# define INLINE inline
#else
# define INLINE
#endif


#define Rods_dist 20.
#define PI 3.141592653589793
#define Helical_Rise        3.4
#define Helical_Rise_inv    0.2941
#define Gaz_const_kcal      0.001987
#define Backbone_length     6.4
#define Eular_const         0.58


extern double kn(int, double);
extern double expn(int, double);


PRIVATE INLINE double
bjerrum_length(double T)
{
  return 2088.762/T;
};


PRIVATE INLINE double
ionic_strength(double rho)
{
  return rho;
};


PRIVATE INLINE double
kappa_inv(double rho, double T)
{
  return 8.1285/(sqrt(bjerrum_length(T)*ionic_strength(rho)));
};


PRIVATE INLINE double
kappa(double rho, double T)
{
  return sqrt(bjerrum_length(T)*ionic_strength(rho))/8.1285;
};


PRIVATE INLINE double
tau_ds(double T)
{
  double bjerrum_length_inv;

  bjerrum_length_inv = 1/bjerrum_length(T);
  return MIN2(Helical_Rise_inv, bjerrum_length_inv);
};


PRIVATE INLINE double
tau_ss(double T)
{
  double bjerrum_length_inv;

  bjerrum_length_inv = 1/bjerrum_length(T);
  return MIN2(1/Backbone_length, bjerrum_length_inv);
};


PRIVATE INLINE double
pairing_salt_const(double rho, double T)
{
  return 2*Gaz_const_kcal*T*bjerrum_length(T)*Helical_Rise*tau_ds(T)*tau_ds(T);
};


PRIVATE double
approx_hyper(double y)
{
	double a, b, c;

	a = 1/(pow(y,6.)/pow(2*PI,6.)+1);
	b = pow(y, 4.)/(36*pow(PI, 4.)) - pow(y, 3.)/(24*PI*PI) + y*y/(2*PI*PI) - y/2;
	c = log(2*PI/y) - 1.96351;

	return a*b + (1-a)*c;
};


PRIVATE double
loop_salt_aux(double kmlss, int m, double T)
{
	double a, b;

	a =  Gaz_const_kcal * T * bjerrum_length(T) * m * Backbone_length * tau_ss(T) * tau_ss(T);
	b = log(kmlss) - log(PI/2) + Eular_const + approx_hyper(kmlss) + 1/kmlss * (1 - exp(-kmlss) + kmlss*expn(1, kmlss));

	return a*b*100;
};

PUBLIC double
vrna_salt_loop(int m, double rho, double T)
{
	if (m == 0)
		return 0;
	double correction, kmlss, kmlss_ref;
	
	kmlss_ref = kappa(1., T) * m * Backbone_length;
	kmlss = kappa(rho, T) * m * Backbone_length;

	correction = loop_salt_aux(kmlss, m, T) - loop_salt_aux(kmlss_ref, m, T);

	return correction;
	/* return (int) (correction + 0.5 - (correction<0)); */
};

PUBLIC int
vrna_salt_stack(double rho, double T)
{
	double correction, kn_ref, kappa_ref;
	
	/* printf("Stack salt temperature %lf\n", T); */ 
	/* printf("Stack salt concentration %lf\n", rho); */ 

	kappa_ref = kappa(1., T);
	kn_ref = kn(0, Rods_dist*kappa_ref);
	correction = 100*pairing_salt_const(rho, T)*(kn(0, Rods_dist*kappa(rho, T)) - kn_ref);
	/* printf("stack correction %lf\n", correction); */
	return (int) (correction + 0.5 - (correction<0)); /* round salt correction*/
}

/* Adapted from https://stackoverflow.com/a/19040841 */
/* LoopEnergy = m*size + b */
PUBLIC void
vrna_salt_ml(double saltLoop[], int lower, int upper, int* m, int* b)
{
	int sumx, sumxx;
	double y, sumy, sumyy, sumxy, denom, dm, db;

	sumx = sumxx = 0;
	sumy = sumyy = sumxy = 0.;

	for (int i=lower; i<=upper; i++)
	{ 
		sumx += i;
		sumxx += i * i;
		
		y = saltLoop[i];

		sumxy += i * y;
		sumy  += y;    
		sumyy += y*y;
	}

	denom = (double) ((upper-lower+1)*sumxx - sumx*sumx);
	dm = ((upper-lower+1) * sumxy  -  sumx * sumy) / denom;
	db = (sumy * sumxx  -  sumx * sumxy) / denom;

	/* printf("Double m %lf\n", dm); */
	/* printf("Double b %lf\n", db); */

	*m = (int) (dm + 0.5 - (dm<0));
	*b = (int) (db + 0.5 - (db<0));
	
}

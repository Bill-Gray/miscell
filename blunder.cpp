#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/* Demonstrates both "blunder management" and kurtosis techniques
for removing or mitigating the effects of outliers in data that is
normally distributed,  but has some "wrong" values in it that should
be excluded.  See http://www.projectpluto.com/blunder.htm for a
description of what this code is doing. */

static double mean_value( const unsigned n_obs, const double *obs)
{
   double sum = 0.;
   unsigned i;

   for( i = 0; i < n_obs; i++)
      sum += obs[i] - obs[0];
   return( obs[0] + sum / (double)n_obs);
}

static double kurtosis( const unsigned n_obs, const double *obs,
                              unsigned *max_idx)
{
   double sum4 = 0., sum2 = 0.;
   const double mean = mean_value( n_obs, obs);
   double delta2_max = 0.;
   unsigned i;

   for( i = 0; i < n_obs; i++)
      {
      const double delta = obs[i] - mean;
      const double delta2 = delta * delta;

      sum2 += delta2;
      sum4 += delta2 * delta2;
      if( delta2_max < delta2)
         {
         *max_idx = i;
         delta2_max = delta2;
         }
      }
   return( (double)n_obs * sum4 / (sum2 * sum2) - 3.);
}

int main( const int argc, const char **argv)
{
   double x[40], x0 = 0., sigma = 0.;
   double blunder_probability = 0.02, kurt;
   unsigned n = (unsigned)argc - 1;
   unsigned i, iter;

   for( i = 0; i < n; i++)
      x[i] = atof( argv[i + 1]);
   x0 = sigma = 0.;

   for( iter = 0; iter < 200; iter++)
      {
      double weight_sum = 0., dxsum = 0., dx2sum = 0., new_sigma;

      for( i = 0; i < n; i++)
         {
         const double dx = x[i] - x0;
         const double z = (iter ? exp( -dx * dx / (sigma * sigma)) : 1);
         const double weight = z / (z + blunder_probability);

         weight_sum += weight;
         dxsum += dx * weight;
         dx2sum += dx * dx * weight;
         }
      dxsum /= weight_sum;
      dx2sum /= weight_sum;
      new_sigma = sqrt( dx2sum - dxsum * dxsum);
      x0 += dxsum;
      printf( "Iter %u: x0 = %f, sigma = %f; changes %f, %f\n", iter,
               x0, new_sigma, dxsum, new_sigma - sigma);
      if( fabs(new_sigma - sigma) / new_sigma < 1.e-5)
         if( fabs( dxsum / new_sigma) < 1.e-5)
            break;
      sigma = new_sigma;
      }
   do
      {
      kurt = kurtosis( n, x, &i);
      if( kurt > 0.)
         {
         n--;
         x[n] = x[i];
         }
      printf( "kurt %f, idx %u: new mean %f\n", kurt, i, mean_value( n, x));
      }
      while( kurt > 0.);
   return( 0);
}

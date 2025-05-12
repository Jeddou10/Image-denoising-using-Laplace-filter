#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <immintrin.h>
#include "denoise.h"


void
denoise_naive (const uint8_t * img, size_t width,
			   size_t height, float a, float b,
			   float c, uint8_t * tmp1, uint8_t * tmp2, uint8_t * result)
{

  // generate Q (tmp1[x+y*width]= Q(x,y))
  float sum = a + b + c;
  for (size_t i = 0; i < width * height * 3; i += 3)
	{
	  tmp1[i / 3] = (a * img[i] + b * img[i + 1] + c * img[i + 2]) / sum;
	}


  // generateQw (tmp2[x+y*width]= Qw(x,y))
  float Mw[9] = { 1, 2, 1, 2, 4, 2, 1, 2, 1 };
  for (size_t y = 0; y < height; y++)
	{
	  for (size_t x = 0; x < width; x++)
		{
		  // for each Pixel with coordinates (x,y) calculate Qw(x,y)
		  float Qw = 0.0;
		  for (size_t i = 0; i <= 2; i++)
			{
			  for (size_t j = 0; j <= 2; j++)
				{
				  if (x + i < width + 1 && x + i >= 1 && y + j >= 1
					  && y + j < height + 1)
					{
					  Qw +=
						Mw[(i) * 3 +
						   j] * (tmp1[x + i - 1 + (y + j - 1) * width]);
					}
				}
			}
		  tmp2[y * width + x] = Qw / (float) 16;
		}
	}


  // generate Ql and Qprime(result[x+y*width] = Qprime(x,y))
  float Ml[9] = { 0, 1, 0, 1, -4, 1, 0, 1, 0 };
  uint8_t Qprime, Qw, d;
  float Ql1020;
  for (size_t y = 0; y < height; y++)
	{
	  for (size_t x = 0; x < width; x++)
		{
		  // for each Pixel with coordinates (x,y) calculate Ql(x,y)
		  float Ql = 0.0;
		  for (size_t i = 0; i <= 2; i++)
			{
			  for (size_t j = 0; j <= 2; j++)
				{
				  if (x + i < width + 1 && x + i >= 1 && y + j >= 1
					  && y + j < height + 1)
					{

					  Ql +=
						Ml[(i) * 3 +
						   j] * (tmp1[x + i - 1 + (y + j - 1) * width]);
					}
				}
			}

		  // for each Pixel with coordinates (x,y) extract d=Q(x,y)  
		  d = tmp1[y * width + x];
		  Qw = tmp2[y * width + x];
		  Ql1020 = Ql > 0 ? Ql / (float) 1020 : (-Ql) / (float) 1020;
		  // for each Pixel with coordinates (x,y) use Q(x,y), Ql(x,y) and Qw(x,y) to calculate Qprime(x,y)
		  Qprime = Ql1020 * d + (1 - Ql1020) * Qw;
		  result[y * width + x] = Qprime;
		}
	}
}


//-------------------------------------------------------------------------------------------------------------------------



void
denoise_optimised (const uint8_t * img, size_t width,
				   size_t height, float a, float b,
				   float c, uint8_t * tmp1, uint8_t * tmp2, uint8_t * result)
{

  // generate Q (tmp1[x+y*width]= Q(x,y))
  float sum = a + b + c;
  for (size_t i = 0; i < width * height * 3; i += 3)
	{
	  tmp1[i / 3] = (a * img[i] + b * img[i + 1] + c * img[i + 2]) / sum;
	}

  // generate Qprime (result[x+y*width] = Qprime(x,y))
  float Ml[9] = { 0, 1, 0, 1, -4, 1, 0, 1, 0 };
  float Mw[9] = { 1, 2, 1, 2, 4, 2, 1, 2, 1 };
  uint8_t Qprime;
  float Ql, Qw, d, Ql1020;
  for (size_t y = 0; y < height; y++)
	{
	  for (size_t x = 0; x < width; x++)
		{
		  // for each Pixel with coordinates (x,y) calculate Ql(x,y) and Qw(x,y)
		  Ql = 0.0;
		  Qw = 0.0;
		  for (size_t j = 0; j <= 2; j++)
			{
			  for (size_t i = 0; i <= 2; i++)
				{
				  if (x + i < width + 1 && x + i >= 1 && y + j >= 1
					  && y + j < height + 1)
					{
					  d = tmp1[x + i - 1 + (y + j - 1) * width];
					  Ql += Ml[3 * i + j] * d;
					  Qw += Mw[3 * i + j] * d;
					}
				}
			}
           
		  // for each Pixel with coordinates (x,y) extract d=Q(x,y) 
		  d = tmp1[x + y * width];
		  Qw /= (float) 16;
		  Ql1020 = Ql > 0 ? (Ql / (float) 1020) : ((-Ql) / (float) 1020);
		  // for each Pixel with coordinates (x,y) use Q(x,y), Ql(x,y) and Qw(x,y) to calculate Qprime(x,y)
		  Qprime = Ql1020 * d + (1 - Ql1020) * Qw;
		  result[y * width + x] = Qprime;
		}
	}
}


//-------------------------------------------------------------------------------------------------------------------------



// tmp1: width*height floats 
void
denoise_simd (const uint8_t * img, size_t width,
			  size_t height, float a, float b,
			  float c, uint8_t * tmp1, uint8_t * tmp2, uint8_t * result)
{

  // fill the first line of tmp1 all with 0
  for (size_t i = 0; i <= width; i++)
	{
	  ((float *) tmp1)[i] = 0;
	}
   //index used to fill tmp1
     size_t index = width + 1 ;
  // fill tmp1 with ds (the first column of tmp1 is all with 0)
  float sum = a + b + c;
  for (size_t i = 0; i < (width * height * 3); i += 3)
	{
	  // fill the first column only  with 0
	  if (index % (width + 1) == 0)
		{
		  ((float *) tmp1)[index] = 0;
		  index++;
		}
	  // calculate d for each Pixel and fill tmp1 with it 
	  ((float *) tmp1)[index] = ((a * img[i] + b * img[i + 1] +
								  c * img[i + 2]) / sum);
	  index++;
	}

  // add a line at the end of tmp1 all with 0
  for (size_t i = index; i <= (index + width + 2); i++)
	((float *) tmp1)[i] = 0;

  // fill Ml and Mw and add a 0 after each 3 elements
  float Ml[12] = { 0, 1, 0, 0, 1, -4, 1, 0, 0, 1, 0, 0 };
  float Mw[12] = { 1, 2, 1, 0, 2, 4, 2, 0, 1, 2, 1, 0 };

  // load each 4 elements of Ml and Mw together using simd
  __m128 Ml1 = _mm_loadu_ps (&Ml[0]);
  __m128 Ml2 = _mm_loadu_ps (&Ml[4]);
  __m128 Ml3 = _mm_loadu_ps (&Ml[8]);
  __m128 Mw1 = _mm_loadu_ps (&Mw[0]);
  __m128 Mw2 = _mm_loadu_ps (&Mw[4]);
  __m128 Mw3 = _mm_loadu_ps (&Mw[8]);

  __m128 Q1, Q2, Q3, mul1, mul2, mul3, add, res_vec;
  float *res;
  float Ql, Qw, d;
  uint8_t Qprime;

  // iterate over all pixels
  for (size_t y = 0; y < height; y++)
	{
	  for (size_t x = 0; x < width; x++)
		{
		  // load each 3 elements from tmp1 in Q1,Q2 and Q3 (the 4 th element will be multiplied by 0 in the next steps and ignored)
		  Q1 =
			_mm_loadu_ps (&((float *) tmp1)
						  [1 + x - 1 + (1 + y - 1) * (width + 1)]);
		  Q2 =
			_mm_loadu_ps (&((float *) tmp1)
						  [1 + x - 1 + (1 + y) * (width + 1)]);
		  Q3 =
			_mm_loadu_ps (&((float *) tmp1)
						  [1 + x - 1 + (1 + y + 1) * (width + 1)]);

		  // scalar multiplication of MLi and Qi
		  mul1 = _mm_mul_ps (Ml1, Q1);
		  mul2 = _mm_mul_ps (Ml2, Q2);
		  mul3 = _mm_mul_ps (Ml3, Q3);
		  // addition of the obatined scalar products
		  add = _mm_add_ps (mul1, mul2);
		  res_vec = _mm_add_ps (add, mul3);
		  // calculate Ql(x,y)
		  res = (float *) &res_vec;
		  Ql = res[0] + res[1] + res[2];

		  // scalar multiplication of Mwi and Qi
		  mul1 = _mm_mul_ps (Mw1, Q1);
		  mul2 = _mm_mul_ps (Mw2, Q2);
		  mul3 = _mm_mul_ps (Mw3, Q3);
		  // addition of the obatined scalar products
		  add = _mm_add_ps (mul1, mul2);
		  res_vec = _mm_add_ps (add, mul3);
		  // calculate Qw(x,y)
		  res = (float *) &res_vec;
		  Qw = res[0] + res[1] + res[2];
		  Qw /= (float) 16;

		  // extraction of d=Q(x,y) from tmp1
		  d = ((float *) tmp1)[1 + x + (1 + y) * (width + 1)];

		  // calculate Q'(x,y)
		  Ql = Ql > 0 ? (Ql / (float) 1020) : ((-Ql) / (float) 1020);
		  Qprime = Ql * d + (1 - Ql) * Qw;
		  // fill result with the calculated Q' (x, y) 
		  result[y * width + x] = Qprime;
		}

	}
}


//-------------------------------------------------------------------------------------------------------------------------




// data to be given to thread's methods
struct ThreadData
{
  uint8_t *Q;
  float *Ql;
  float *Qw;
  uint8_t *result;
  size_t width;
  size_t height;
}
 ;

// (producer )method to calculate Ql
void *
calculate_Ql (void *arg)
{
  struct ThreadData *data = (struct ThreadData *) arg;
  size_t width = data->width;
  size_t height = data->height;
  uint8_t *Q = data->Q;
  float *Ql = data->Ql;

  float Ml[9] = { 0, 1, 0, 1, -4, 1, 0, 1, 0 };
  float res;
  for (size_t y = 0; y < height; y++)
	{
	  for (size_t x = 0; x < width; x++)
		{
		  res = 0.0;
		  for (size_t j = 0; j <= 2; j++)
			{
			  for (size_t i = 0; i <= 2; i++)
				{
				  if (x + i < width + 1 && x + i >= 1 && y + j >= 1
					  && y + j < height + 1)
					{
					  res +=
						Ml[(i) * 3 + j] * Q[x + i - 1 + (y + j - 1) * width];
					}
				}
			}
		  Ql[y * width + x] =
			(res > 0 ? (res / (float) (1020)) : (-res) / (float) (1020));
		}
	}
  return NULL;
}

//(producer ) method to calculate Qw
void *
calculate_Qw (void *arg)
{
  struct ThreadData *data = ((struct ThreadData *) arg);
  size_t width = data->width;
  size_t height = data->height;
  uint8_t *Q = data->Q;
  float *Qw = data->Qw;

  float Mw[9] = { 1, 2, 1, 2, 4, 2, 1, 2, 1 };
  float res;
  for (size_t y = 0; y < height; y++)
	{
	  for (size_t x = 0; x < width; x++)
		{
		  res = 0.0;
		  for (size_t j = 0; j <= 2; j++)
			{
			  for (size_t i = 0; i <= 2; i++)
				{
				  if (x + i < width + 1 && x + i >= 1 && y + j >= 1
					  && y + j < height + 1)
					{
					  res +=
						Mw[(i) * 3 + j] * Q[x + i - 1 + (y + j - 1) * width];
					}
				}
			}
		  Qw[y * width + x] = res / (float) 16;
		}
	}
  return NULL;
}

//(consumer) method to calculate the result 
void *
calculate_result (void *arg)
{
  struct ThreadData *data = ((struct ThreadData *) arg);
  size_t width = data->width;
  size_t height = data->height;
  uint8_t *Q = data->Q;
  float *Ql = data->Ql;
  float *Qw = data->Qw;
  uint8_t *result = data->result;
  uint8_t Qprime;
  for (size_t index = 0; index < width * height; index++)
	{
	  // waiting for calcculating Ql and Qw
	  while (Ql[index] == -1 || Qw[index] == -1);
	  Qprime = Ql[index] * Q[index] + (1 - Ql[index]) * Qw[index];
	  result[index] = Qprime;
	}
  return NULL;
}

void
denoise_threading (const uint8_t * img, size_t width,
				   size_t height, float a, float b,
				   float c, uint8_t * tmp1, uint8_t * tmp2, uint8_t * result)
{

 
  float * Qw = malloc(width*height*4);
  if(Qw==NULL){
	fprintf(stderr, "Error Allocation\n");
	exit(EXIT_FAILURE);
  }
  for (size_t i = 0; i < width*height; i++)
	{
	  ((float *) tmp2)[i] = -1;
	  Qw[i] = -1;
	}
  pthread_t threads[3];
  float sum = a + b + c;
  for (size_t i = 0; i < width * height * 3; i += 3)
	{
	  tmp1[i / 3] = (a * img[i] + b * img[i + 1] + c * img[i + 2]) / sum;
	}
  struct ThreadData data = (struct ThreadData)
  {
	.Q = tmp1,
	.Ql = (float *) tmp2,
	.Qw = Qw,
	.result = result,
	.width = width,
	.height = height
  };

  pthread_create (&threads[0], NULL, calculate_Ql, &data);
  pthread_create (&threads[1], NULL, calculate_Qw, &data);
  pthread_create (&threads[2], NULL, calculate_result, &data);
  pthread_join (threads[0], NULL);
  pthread_join (threads[1], NULL);
  pthread_join (threads[2], NULL);
  free(Qw);
}







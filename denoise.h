void denoise_naive (const uint8_t * img, size_t width,
	  size_t height, float a, float b,
	  float c, uint8_t * tmp1, uint8_t * tmp2, uint8_t * result);


void denoise_optimised (const uint8_t * img, size_t width,
	  size_t height, float a, float b,
	  float c, uint8_t * tmp1, uint8_t * tmp2, uint8_t * result);

	  
void denoise_simd ( const uint8_t * img, size_t width,
	  size_t height, float a, float b,
	  float c, uint8_t * tmp1, uint8_t * tmp2, uint8_t * result);


void denoise_threading (const uint8_t * img, size_t width,
	   size_t height, float a, float b,
	   float c, uint8_t * tmp1, uint8_t * tmp2, uint8_t * result);
#define _POSIX_C_SOURCE 199309L
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "denoise.h"
#include "benchmark.h"

// run the method testCycles time and calculate the time 
void implementation(
        long testCycles, 
        const uint8_t* img,
        size_t width,
        size_t height,
        float a,
        float b,
        float c,
        uint8_t* tmp1,
        uint8_t* tmp2,
        uint8_t* result,
        void implement(const uint8_t* , size_t ,size_t ,float ,float ,float ,uint8_t* ,uint8_t* ,uint8_t*)
) {
    struct timespec start;
    int err = clock_gettime(CLOCK_MONOTONIC, &start);
    if (err != 0) {
        printf("[!] failed to get current time\n");
        exit(EXIT_FAILURE);
    }

    for (long i = 0; i < testCycles; i++) {
        implement(img, width,  height , a ,b, c , tmp1 , tmp2 , result);
    }

    struct timespec end;
    err = clock_gettime(CLOCK_MONOTONIC, &end);
    if (err != 0) {
        printf("[!] failed to get current time\n");
        exit(EXIT_FAILURE);
    }

    long time_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    long avg_time_ns = time_ns / testCycles;
    long time_pro_pixel = avg_time_ns / (width*height);
    double seconds = time_ns * 1e-9;

    printf("[i] absolute time: %f s\n", seconds);
    printf("[i] average time:  %ld ns\n", avg_time_ns);
    printf("[i] time pro pixel:  %ld ns\n", time_pro_pixel);
    
}
//allocate the needed space for each method and run it 
void bench_implementation(
        long testCycles, 
        const uint8_t* img,
        size_t width,
        size_t height,
        float a,
        float b,
        float c,
        uint8_t* result,
        int version
        ){
    uint8_t* tmp1 = NULL;
    uint8_t* tmp2 = NULL;
            switch (version)
    {
    //denoise_naive
    case 0:
            if(!(tmp1 = malloc(width*height))) {
           fprintf(stderr, "Error creating temp buffer 1: Could not allocate enough memory\n");
           exit( EXIT_FAILURE);
           }
            if(!(tmp2 = malloc(width*height))) {
            fprintf(stderr, "Error creating temp buffer 2: Could not allocate enough memory\n");
            exit( EXIT_FAILURE);
            }
        fprintf(stderr, "Running denoise_naive %ld time ...\n",testCycles);
        implementation(
         testCycles, 
         img,
         width,
         height,
         a,
         b,
         c,
         tmp1,
         tmp2,
         result,
        denoise_naive
        );
        fprintf(stderr, "Done!...\n");            
        free(tmp1);
        free(tmp2);
            break;

    //denoise_optimised
    case 1:
        
        if(!(tmp1 = malloc(width*height))) {
           fprintf(stderr, "Error creating temp buffer 1: Could not allocate enough memory\n");
           exit( EXIT_FAILURE);
           }
        fprintf(stderr, "Running denoise_optimised %ld time ...\n",testCycles);
        implementation(
         testCycles, 
         img,
         width,
         height,
         a,
         b,
         c,
         tmp1,
         tmp2,
         result,
        denoise_optimised
        );
        fprintf(stderr, "Done!...\n");
        free(tmp1);
        break;
    //denoise_threading
    case 2:
          if(!(tmp1 = malloc(width*height))) {
           fprintf(stderr, "Error creating temp buffer 1: Could not allocate enough memory\n");
            exit( EXIT_FAILURE);
           }
            if(!(tmp2 = malloc(width*height*4))) {
            fprintf(stderr, "Error creating temp buffer 2: Could not allocate enough memory\n");
            exit( EXIT_FAILURE);
            }
        fprintf(stderr, "Running denoise_threading %ld time ...\n",testCycles);
         implementation(
         testCycles, 
         img,
         width,
         height,
         a,
         b,
         c,
         tmp1,
         tmp2,
         result,
        denoise_threading
        );
        fprintf(stderr, "Done!...\n");
        free(tmp1);
        free(tmp2);
        break;
    //denoise_simd
    case 3:
     if(!(tmp1 = malloc(((width+1)*(height+2)+2)*4))) {
            fprintf(stderr, "Error creating temp buffer 1: Could not allocate enough memory\n");
            exit(EXIT_FAILURE);
            }
         fprintf(stderr, "Running denoise_simd %ld time ...\n",testCycles);
         implementation(
         testCycles, 
         img,
         width,
         height,
         a,
         b,
         c,
         tmp1,
         tmp2,
         result,
        denoise_simd
        );
        fprintf(stderr, "Done!...\n");
        free(tmp1);
        break;

    default:
        fprintf(stderr,"nonexistent version (unvalid command -V%d), please choose a version between 0 and 3 ...\n",version);
        exit(EXIT_FAILURE);
        break;
    }
}
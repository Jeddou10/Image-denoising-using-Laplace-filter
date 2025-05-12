#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include "denoise.h"
#include "benchmark.h"

static void print_help() {
    static char* helptext =     "\na220_implementation\n\n"
                                "DESCRIPTION\n"
                                "a220_implementation is used to convert a given color picture of P6 PPM format into grey scales and afterwards use a"
                                "combination of the Laplace Filter and blurring to denoise the picture.\n"
                                "There are 4. possible Implementations to choose from:\n"
                                "1. -V 0 or no -V: naive Implementation.\n"
                                "2. -V 1: optimised Implementation. Uses the order of Matrice calculations to be quicker.\n"
                                "3. -V 2: threaded Implementation. Uses multiple Threads to calculate Matrices parallel.\n"
                                "4. -V 3: SIMD Implementation. Uses SIMD operations for quicker Matrice calculations.\n\n"
                                "OPTIONS\n"
                                "-v, -V, --V\n"
                                "Which of the 4 different Implementations that are descripted above is run. Accepts an Integer Value between 0 and 3.\n\n"
                                "-b, -B, --B\n"
                                "Runtime is meassured and argument defines how often the Implementation is run in repetition. Accepts no Argument or Integer above 0.\n\n"
                                "-o\n"
                                "The name for the output file with .pgm ending\n\n"
                                "--coeffs\n"
                                "The 3 coefficients used in the grey scale calculation. Takes 3 float Values seperated by \",\" Example: 1.0,2.0,3.0.\n\n"
                                "-h, --help\n"
                                "Display help text and exit. No other output is generated.\n\n"
                                "OUTPUT\n"
                                "A single Output file of netpbm format is generated. The name of the file is defined by the executor .\n\n"
                                "EXAMPLES\n"
                                "./a220_implementation -V 1 -B 3 mandrillKopie.ppm -o output.pgm --coeffs 0.2126,0.7152,0.0722;\n\n" 
                                ;
    fprintf(stderr, "%s", helptext);
}

void writePGM(uint8_t* result , char* output , size_t width , size_t height ){
    //Open output File
    FILE *outputfile = fopen(output, "wb");

    if (!outputfile) {
        fprintf(stderr, "Error opening file %s to write.\n", output);
        exit(1);
    }

    //Write P5, width and height for the P5 Header
    fprintf(outputfile, "P5\n%zu %zu\n255\n", width, height);
    
    //Write the result buffer into the output file
    fwrite(result, sizeof(uint8_t), width * height , outputfile);

    //Close the output file
    fclose(outputfile);
    fprintf(stderr, "Outpufile is successfully generated... \n");

}

int main(int argc, char **argv) {    
    int version = 0;
    long testCycles = 1;
    char* output = NULL;
    float a = 0.2126;
    float b = 0.7152;
    float c = 0.0722;
    size_t width = 0; //rows
    size_t height = 0; //cols
    int benchFlag = 0 ;
     char *pointer;
     char* coeffs;
    static struct option long_options[] = {
        {"V", required_argument, 0, 'v'},
        {"v", required_argument, 0, 'v'},
        {"B", optional_argument, 0, 'b'},
        {"b", optional_argument, 0, 'b'},
        {"o", required_argument, 0, 'o'},
        {"coeffs", required_argument, 0, 'c'},
        {"h", no_argument, 0, 'h'},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0},
    };
    int opt;
    int option_index = 0;

    while((opt = getopt_long(argc, argv, "V:v:B::b::o:c:h", long_options, &option_index)) != -1) {
        char* endptr;
        switch (opt)
        {
        case 'V':
        case 'v':
            version = strtol(optarg, &endptr, 10);
            if(endptr == optarg || *endptr != '\0') {
                //could not be converted to double
                fprintf(stderr, "The Input for the Version ist not correct. Please make sure your input is in order with the rules:\n");
                print_help();
                return EXIT_FAILURE;
            }
            if (version < 0 || version > 3)
            {
                fprintf(stderr, "The Input Value for the Version is not within Bounds. Please refer to our Guidelines:");
                print_help();
                return EXIT_FAILURE;
            }
            break;
        case 'B':
        case 'b':    
            benchFlag = 1;
            //Since b is optional optarg is always NULL if argument is not close (-b1). Check to recocnice number behind -b and make sure it is not another command or the positional filename
            if (optarg == NULL && optind < argc && argv[optind][0] != '-' && argv[optind][strlen(argv[optind])-3] != 'p'&& argv[optind][strlen(argv[optind])-2] != 'p' && argv[optind][strlen(argv[optind])-1] != 'm')
            {
                optarg = argv[optind++];
                char* endptr;
                testCycles = strtol(optarg, &endptr, 10);
                if(endptr == optarg || *endptr != '\0') {
                //could not be converted to double
                fprintf(stderr, "The Input for the Program Repetition Counter ist not correct. Please refer to our Guidelines:\n");
                print_help();
                return EXIT_FAILURE;
                }        
                if (testCycles < 1) {
                    fprintf(stderr, "The Value for the Program Repetition Counter is not within bounds. Please refer to our Guidelines:\n");
                    print_help();
                    return EXIT_FAILURE;
                }
            }               
            break;
        case 'o':
            output = optarg;
            break;
        case 'c':
            coeffs = optarg;
            pointer = strtok(coeffs, ","); 
            //convert value a to float
            a = strtof(pointer, &endptr);
            if(endptr == pointer || *endptr != '\0') {
                //could not be converted to float
                fprintf(stderr, "The Input for the 1. coeff number could not be converted to float. Please refer to our Guidelines:\n");
                print_help();
                return EXIT_FAILURE;
            } else if (errno == ERANGE) {
                fprintf(stderr, "The Input for the 1. coeff number under- or overflows floats. Please refer to our Guidelines:");
                return EXIT_FAILURE;
            }
            //Check that a value is not negativ:
            if (a < 0) {
                fprintf(stderr, "The Input for the 1. coeff number can not be negativ. Please refer to our Guidelines:\n");
                print_help();
                return EXIT_FAILURE;
            }
            //convert value b to float
            pointer = strtok(NULL, ",");
            b = strtof(pointer, &endptr);
            if(endptr == pointer || *endptr != '\0') {
                //could not be converted to float
                fprintf(stderr, "The Input for the 2. coeff number could not be converted to float. Please refer to our Guidelines:\n");
                print_help();
                return EXIT_FAILURE;
            } else if (errno == ERANGE) {
                fprintf(stderr, "The Input for the 2. coeff number under- or overflows floats. Please refer to our Guidelines:");
                return EXIT_FAILURE;
            }
            //Check that b value is not negativ:
            if (b < 0) {
                fprintf(stderr, "The Input for the 2. coeff number can not be negativ. Please refer to our Guidelines:\n");
                print_help();
                return EXIT_FAILURE;
            }
            //convert value c to float
            pointer = strtok(NULL, ",");
            c = strtof(pointer, &endptr);
            if(endptr == pointer || *endptr != '\0') {
                //could not be converted to float
                fprintf(stderr, "The Input for the 3. coeff number could not be converted to float. Please refer to our Guidelines:\n");
                print_help();
                return EXIT_FAILURE;
            } else if (errno == ERANGE) {
                fprintf(stderr, "The Input for the 3. coeff number under- or overflows floats. Please refer to our Guidelines:");
                return EXIT_FAILURE;
            }
            //Check that b value is not negativ:
            if (c < 0) {
                fprintf(stderr, "The Input for the 3. coeff number can not be negativ. Please refer to our Guidelines:\n");
                print_help();
                return EXIT_FAILURE;
            }
            
            break;
        case 'h':
            print_help();
            return EXIT_SUCCESS;
        case '?':
            printf("there is an invalid command.... Please check the guidelines:\n"); 
            print_help();
            break;
        default:
            return EXIT_FAILURE;
        }
    }
    //check for the input filenamen as positional argument
    if (optind == argc) {
        fprintf(stderr, "Missing positional argument -- 'filename'. Please check the guidelines:\n");
        print_help();
        return EXIT_FAILURE;
    }

    //Check for filename as only positional argument
    if (argc > optind + 1) {
        fprintf(stderr, "To many positional arguments. Please check the guidelines:\n");
        print_help();
        return EXIT_FAILURE;
    }

    const char* filename = argv[optind];

    // Check the input file for correct ppm ending
    int fileNameLength = strlen(filename);
    if (filename[fileNameLength-3] != 'p' || filename[fileNameLength-2] != 'p' || filename[fileNameLength-1] != 'm')
    {
        fprintf(stderr, "Input File ending is not .ppm. Please check the guidelines:\n");
        print_help();
        return EXIT_FAILURE;
    }

    //Check for excistance of output name
    if (!output)
    {
        fprintf(stderr, "No Name for the output file. Please check the guidelines:\n");
        print_help();
        return EXIT_FAILURE;
    }

    //Check the output file for corret ppm ending
    int outputFileLength = strlen(output);
    if (output[outputFileLength-3] != 'p' || output[outputFileLength-2] != 'g' || output[outputFileLength-1] != 'm')
    {
        fprintf(stderr, "Output File ending is not .pgm. Please check the guidelines:\n");
        print_help();
        return EXIT_FAILURE;
    }
   
    
    //Read data from input file
    uint8_t* img = NULL;
    FILE* file;

    //Check for Errors while file opening, file stats, filetype and data buffer
    if (!(file = fopen(filename, "r"))) {
        perror("Error opening file");
        fprintf(stderr, "Error opening file\n");
        return EXIT_FAILURE;
    }

    struct stat statbuf;
    if (fstat(fileno(file), &statbuf)) {
        fprintf(stderr, "Error retrieving file stats\n");
        if (file) fclose(file);
        return EXIT_FAILURE;
    }
    if (!S_ISREG(statbuf.st_mode) || statbuf.st_size <= 0)
    {
        fprintf(stderr, "Error processing file: Not a regular file or invalid size\n");
        if (file) fclose(file);
        return EXIT_FAILURE;
    }

   

    // Read the PPM header
    char format[3];
    if(fscanf(file, "%2s\n", format) != 1) {
        fprintf(stderr, "Error reading the file header\n");
        if (file) fclose(file);
        return EXIT_FAILURE;
    };
    if (format[0] != 'P' || format[1] != '6') {
        fprintf(stderr, "Invalid PPM file format. Please check the guidelines:\n");
        print_help();
        if (file) fclose(file);
        return EXIT_FAILURE;
    }

    // Read and ignore comments (lines starting with #)
    char comments = getc(file);
    while (comments == '#') {
        while (getc(file) != '\n');
        comments = getc(file);
    }
    ungetc(comments, file);

    // Read width, height, and max color value
    size_t maxColor;
    if(fscanf(file, "%zu %zu %zu\n", &width, &height, &maxColor)!=3){
        fprintf(stderr, "Error reading width, height and max color of the file\n");
        if (file) fclose(file);
        return EXIT_FAILURE; 
    };
    if(maxColor>255){
         fprintf(stderr, "the input file is not valid. Max color value for pixels is bigger than 255\n");
        if (file) fclose(file);
        return EXIT_FAILURE;
    }
     if(!(img = malloc(width*height*3))) {
        fprintf(stderr, "Error reading file: Could not allocate enough memory\n");
        if (file) fclose(file);
        return EXIT_FAILURE;
    }
    // Read pixel values into the image buffer
    fread(img, sizeof(uint8_t), width * height * 3, file);
    fclose(file);
     uint8_t* result = NULL;
    if(!(result = malloc(width*height))) {
        fprintf(stderr, "Error creating result buffer: Could not allocate enough memory\n");
        if (file) fclose(file);
        return EXIT_FAILURE;
    }
    //benchmarking on 
    if(benchFlag){
    bench_implementation(
         testCycles, 
         img,
         width,
         height,
         a,
         b,
         c,
         result,
         version
        );

    writePGM(result,output,width,height);
    free(result);
    free(img);
        return EXIT_FAILURE;
    }


     uint8_t* tmp1 = NULL;
    uint8_t* tmp2 = NULL;
            switch (version)
    {
    //denoise_naive
    case 0:
            if(!(tmp1 = malloc(width*height))) {
           fprintf(stderr, "Error creating temp buffer 1: Could not allocate enough memory\n");
           return EXIT_FAILURE;
           }
            if(!(tmp2 = malloc(width*height))) {
            fprintf(stderr, "Error creating temp buffer 2: Could not allocate enough memory\n");
            return EXIT_FAILURE;
            }
         fprintf(stderr, "Running denoise_naive ...\n");
        denoise_naive(img,width,height,a,b,c,tmp1,tmp2,result); 
        fprintf(stderr, "Done!...\n");        
        free(tmp1);
        free(tmp2);
            break;

    //denoise_optimised
    case 1:
        
        if(!(tmp1 = malloc(width*height))) {
           fprintf(stderr, "Error creating temp buffer 1: Could not allocate enough memory\n");
           return EXIT_FAILURE;
           }
          fprintf(stderr, "Running denoise_optimised ...\n");
          denoise_optimised(img,width,height,a,b,c,tmp1,tmp2,result);
           fprintf(stderr, "Done!...\n");
        free(tmp1);
        break;
    //denoise_threading
    case 2:
          if(!(tmp1 = malloc(width*height))) {
           fprintf(stderr, "Error creating temp buffer 1: Could not allocate enough memory\n");
           return EXIT_FAILURE;
           }
            if(!(tmp2 = malloc(width*height*4))) {
            fprintf(stderr, "Error creating temp buffer 2: Could not allocate enough memory\n");
            return EXIT_FAILURE;
            }
         fprintf(stderr, "Running denoise_threading ...\n");
        denoise_threading(img,width,height,a,b,c,tmp1,tmp2,result);
        fprintf(stderr, "Done!...\n");
        free(tmp1);
        free(tmp2);
        break;
    //denoise_simd
    case 3:
     if(!(tmp1 = malloc(((width+1)*(height+2)+2)*4))) {
            fprintf(stderr, "Error creating temp buffer 1: Could not allocate enough memory\n");
            return EXIT_FAILURE;
            }
         fprintf(stderr, "Running denoise_SIMD...\n");
        denoise_simd(img,width,height,a,b,c,tmp1,tmp2,result);
        fprintf(stderr, "Done!...\n");
        free(tmp1);
        break;

    default:
        fprintf(stderr,"nonexistent version (unvalid command -V%d), please choose a version between 0 and 3 ...\n",version);
        break;
    }
    
   //write pgm-file
   writePGM(result,output,width,height);
    //Free all allocated memory
    free(result);
    free(img);
}
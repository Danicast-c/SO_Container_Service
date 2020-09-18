/*
 * A simple libpng example program
 * http://zarb.org/~gc/html/libpng.html
 *
 * Modified by Yoshimasa Niwa to make it much simpler
 * and support all defined color_type.
 *
 * To build, use the next instruction on OS X.
 * $ brew install libpng
 * $ clang -lz -lpng16 libpng_test.c
 *
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <string.h>

//Function declaration:
void process_png_file(char *filename);
void read_png_file(char *filename, int not_trusted);
void write_png_file(char *filename, int color);
void erase_image (char *filename);


int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers = NULL;



int main(int argc, char *argv[]) {
  if(argc != 2) abort();

  read_png_file(argv[1],0);

  //erase_image (argv[1]);


  return 0;
}


void read_png_file(char *filename, int not_trusted) {
  FILE *fp = fopen(filename, "rb");

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png) abort();

  png_infop info = png_create_info_struct(png);
  if(!info) abort();

  if(setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  png_read_info(png, info);

  width      = png_get_image_width(png, info);
  height     = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth  = png_get_bit_depth(png, info);

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt

  if(bit_depth == 16)
    png_set_strip_16(png);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

  if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

  png_read_update_info(png, info);

  if (row_pointers) abort();

  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  png_read_image(png, row_pointers);

  fclose(fp);

  //Destroy to avoid memory leak
  png_destroy_read_struct(&png, &info, NULL);

  //Call the next process
  if (not_trusted==1) write_png_file(filename, 3);
  else process_png_file(filename);
  }

void write_png_file(char *filename, int color) {
  int y;

  //Chooses the correct directory to store the image
  char* location;
  char route[80];
  if (color == 0) location = "images/R/";
  else if (color == 1) location = "images/G/";
  else if (color == 2) location = "images/B/";
  else if (color == 3) location = "images/not_trusted/";
  else {
    printf("Wrong color, route not asigned\n");
    abort();
  }
  sprintf(route,"%s%s",location,filename);
  printf("Route to store the image: %s\n",route);
  
  FILE *fp = fopen(route, "wb");
  if(!fp) {
    printf("Impossible to create the stored image\n");
    abort();
  }

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();

  png_infop info = png_create_info_struct(png);
  if (!info) abort();

  if (setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
    png,
    info,
    width, height,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);

  // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
  // Use png_set_filler().
  //png_set_filler(png, 0, PNG_FILLER_AFTER);

  if (!row_pointers) abort();

  png_write_image(png, row_pointers);
  png_write_end(png, NULL);

  for(int y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);

  fclose(fp);

  //Destroy to avoid memory leak
  png_destroy_write_struct(&png, &info);


}

void process_png_file(char *filename) {
  int R = 0;
  int G = 0;
  int B = 0;
  int color;
  for(int y = 0; y < height; y++) {
    png_bytep row = row_pointers[y];
    for(int x = 0; x < width; x++) {
      png_bytep px = &(row[x * 4]);
      // Do something awesome for each pixel here...

      R+=px[0];
      G+=px[1];
      B+=px[2];

      //printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);
    }
  }
  printf("Total de color: R:%d, G:%d, B:%d\n",R,G,B);

  //Indentifies the predominant color
  if (R >= G && R >= B){
    color = 0;
    printf("R is the predominant color.\n", R);
  }
  else if (G >= R && G >= B){
    color = 1;
    printf("R is the predominant color.\n", G);
  }
  else if (B >= R && B >= G){
    color = 2;
    printf("R is the predominant color.\n", B);
  }
  else {
    printf("Problem identifying the color\n");
    abort();
  }

  write_png_file(filename, color);
}

void erase_image (char *filename){
  if (remove(filename) == 0) 
      printf("Deleted successfully\n"); 
  else
      printf("Unable to delete the file\n");
}
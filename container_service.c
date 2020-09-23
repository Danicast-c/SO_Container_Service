#include<stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <png.h>
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

//Functions declaration
void create_files();
void process_png_file(char *filename, int width, int height, png_bytep *row_pointers);
void read_png_file(char *filename, int cont , int not_trusted);
void write_png_file(char *filename, int color,int width, int height, png_bytep *row_pointers);
void erase_image (char *filename);

int main() {
  //Variable Declaration
  int fd =0, confd = 0,b,tot;
  int cont = 0;
  int true_val = 1;
  char buff[1025];
  char* filename = "datavolume/configuracion.config";
  char not_trusted[13]= "not_trusted:";
  struct sockaddr_in serv_addr;
  struct sockaddr_in c_addr;
  struct sockaddr_in client_addr;
  struct stat st = {0};

  //Verify that the datavolume is mounted on the container
  if (stat("datavolume", &st) == -1){
    printf("Aborting: The datavolume is not mounted\n");
    abort();
  }

  //Verify that the config file exist
  FILE *config_file;
    config_file = fopen(filename, "r");
    if (config_file == NULL){
        printf("Aborting: configuracion file don't exist\n");
        abort();
    }
  fclose(config_file);

  //Create the images files on the container
  create_files();

  //SERVER START
  client_addr.sin_family = AF_INET;
  socklen_t c_len = sizeof(client_addr);
  fd = socket(AF_INET, SOCK_STREAM, 0);
  printf("Socket created\nWaiting for client...\n");
  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(buff, '0', sizeof(buff));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(5000);
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &true_val, sizeof(int));
  bind(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
  listen(fd, 10);
 
  while(1){
    int on_config= 0;
    int trusted = 1;
    char ip[20];
    char str[20];

    confd = accept(fd, (struct sockaddr*)&client_addr, &c_len);

    printf("Client connected: %s:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
    strcpy(ip, inet_ntoa(client_addr.sin_addr));
    
    FILE *config_file;
    config_file = fopen(filename, "r");
    if (config_file == NULL){
      printf("Aborting: Config file is lost(?)\n");
      abort();
    }
    //Verify the status of the ip
    while (fgets(str, 20, config_file) != NULL){
      str[strlen(str) -1] = '\0';
      //printf("Comparing: %s VS %s\n",str,ip );
      //Verify the not_trusted ip
      if (strcmp(str, not_trusted) == 0) trusted = 2;
      //If ip is on file
      else if (strcmp(str, ip) == 0){
        on_config = trusted;
        if (on_config == 1){
          printf("IP is on list\n");
        } else {
          printf("IP is not trusted\n");
        }
        break;
      }
    }
    fclose(config_file);

    //if the ip isn't on the config file
    if (on_config == 0){
      close(confd);
      printf("Unrecognized IP: Shutting down connection\n\nWaiting for a new client...\n");
    } else {
    //if the ip is in the file
      if (confd==-1) {
        perror("Accept");
        continue;
      }

      FILE* fp = fopen( "temp.png", "wb");
      tot=0;
      if(fp != NULL){
        while( (b = recv(confd, buff, 1024,0))> 0 ) {
          tot+=b;
          fwrite(buff, 1, b, fp);
        }
        //Recieve the image
        printf("Received byte: %d\n",tot);
        if (b<0) perror("Receiving");
        fclose(fp);
      } else {
        perror("File");
      }
      close(confd);

      //If client type fin
      if (tot==0){
        printf("Client disconected\n\n" );
      } else{
        //The image is ready to process
        cont +=1;
        read_png_file("temp.png", cont,on_config);
        erase_image("temp.png");
      }
    } 
  }
  return 0;
}


void process_png_file(char *filename, int width, int height, png_bytep *row_pointers) {
  int R = 0;
  int G = 0;
  int B = 0;
  int color;
  for(int y = 0; y < height; y++) {
    png_bytep row = row_pointers[y];
    for(int x = 0; x < width; x++) {
      png_bytep px = &(row[x * 4]);
      // Pixel color counter
      R+=px[0];
      G+=px[1];
      B+=px[2];
    }
  }
  //printf("Total de color: R:%d, G:%d, B:%d\n",R,G,B);

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
  write_png_file(filename, color , width, height, row_pointers);
  
}

void erase_image (char *filename){
  if (remove(filename) != 0) 
      printf("Unable to delete the file\n");
}


void read_png_file(char *filename, int cont ,int not_trusted) {

  int width, height;
  png_byte color_type;
  png_byte bit_depth;
  png_bytep *row_pointers = NULL;

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

  //Choose new image name
  char new_name[25];
  sprintf(new_name,"image_%d.png",cont);
  //Destroy to avoid memory leak
  png_destroy_read_struct(&png, &info, NULL);

  //Call the next process
  if (not_trusted==2) write_png_file(new_name, 3 , width, height, row_pointers);
  else process_png_file(new_name, width, height, row_pointers);
}

void write_png_file(char *filename, int color, int width, int height, png_bytep *row_pointers) {
  int y;

  //Chooses the correct directory to store the image
  char* location;
  char route[80];
  if (color == 0) location = "datavolume/images/R/";
  else if (color == 1) location = "datavolume/images/G/";
  else if (color == 2) location = "datavolume/images/B/";
  else if (color == 3) location = "datavolume/images/not_trusted/";
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

void create_files(){
// Create files to store the images
  struct stat st = {0};
  if (stat("datavolume/images", &st) == -1) {
      //mkdir("datavolume", 0700);
      mkdir("datavolume/images", 0700);
      mkdir("datavolume/images/R", 0700);
      mkdir("datavolume/images/G", 0700);
      mkdir("datavolume/images/B", 0700);
      mkdir("datavolume/images/not_trusted", 0700);
  } else {
    printf("The images files already exist\n");
  }

}
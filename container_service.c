#include<stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
//printf("\nA sample C program\n\n");

// Create files to store the images
struct stat st = {0};
if (stat("images", &st) == -1) {
    mkdir("images", 0700);
    mkdir("images/R", 0700);
    mkdir("images/G", 0700);
    mkdir("images/B", 0700);
    mkdir("images/not_trusted", 0700);
} else {
	printf("\nThe images files already exist\n\n");
}
//printf("%d \n",result );

return 0;
}
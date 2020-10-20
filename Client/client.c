#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){
    // if (argc != 2){
    //     printf("IP not entered");
    //     abort();
    // }
    //Variable declaration
    int sfd =0, n=0, b;
    char rbuff[1024];
    char sendbuffer[100];
    char fin[5] = "fin\0";
    struct sockaddr_in serv_addr;

    while(1){
        //Start Client
        memset(rbuff, '0', sizeof(rbuff));
        sfd = socket(AF_INET, SOCK_STREAM, 0);

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(5000);
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
        //Create the connection
        b=connect(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (b==-1) {
            perror("Connect");
            return 1;
        }
   
        char dir[100];
        //Get the directory
        printf("Enter a directory: ");
        if (fgets(dir, sizeof(dir), stdin) == NULL) { 

            perror("fgets error");
            return 2;
        }
        
        dir[strlen(dir) -1] = '\0'; 
        printf("You entered: %s\n", dir);

        //Ends client if fin is typed
        if (strcmp(dir, fin) == 0){
            printf("Connection ended.\n" );
            return 0;
        }

        //Opens the png
        FILE *fp = fopen(dir, "rb");
        if(fp == NULL){
            perror("File");
            return 2;
        }

        //Sends the png to the server
        while( (b = fread(sendbuffer, 1, sizeof(sendbuffer), fp))>0 ){
            send(sfd, sendbuffer, b, 0);
        }

        close(sfd);
        fclose(fp);
    }   
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
void encrypt(char password[],int key){
    for (int i=0;i<strlen(password);i++) {
        password[i]=password[i]-key;
    }
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    

    // //Real processing starts here
    // printf("1.Login\n2.Exit\nEnter either 1 or 2\n");
    // int cas;
    // scanf("%d",&cas);
    // if(cas==2)exit(1); //exit

    // //Validating username
    // char validusername[]="tanuj";
    // printf("Enter Username: \n");
    // char username[20];
    // scanf("%s", username);
    // if(strcmp(username,validusername)!=0){
    //     printf("Username doesn't exist");
    //     exit(1); //exit
    // }

    // //Validating password
    // char validpassword[]="1234";
    // printf("Enter your password: \n");
    // char password[20];
    // scanf("%s",password);
    // if(strcmp(password,validpassword)!=0){
    //     printf("Password invlid");
    //     exit(1); //exit
    // }

    printf("ready to write");
    bzero(buffer,256);
    fgets(buffer,20,stdin);
    n = write(sockfd,buffer,strlen(buffer)-1);  // send username
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,4);  // receive rand no. (key)
    if (n < 0) 
         error("ERROR reading from socket");
    int key = ntohl(*(int*) buffer);
    bzero(buffer,256);
    fgets(buffer,20,stdin);
    encrypt(buffer,key);
    n = write(sockfd,buffer,strlen(buffer)-1); // return encrypted password
    if (n < 0) 
         error("ERROR writing to socket");
    int size=read(sockfd,buffer,20);  // 
    printf("d",size);
    printf("%d %d %d",n,*(int*) buffer,key);
    printf("%d\n",*(int*)buffer);

    close(sockfd);
    return 0;
}
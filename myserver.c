#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

int sockfd, newsockfd, portno, status;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

char buffer[256];
char dbase1[2000];
char dbase2[2000];

FILE* db1;
FILE* db2;

void adduser(char* username,char* password,char auth) {
    char buff[41];
    bzero(buff,41);
    int n=strlen(username);
    if (n>19) {
        perror("limit exceeded");
    }
    memcpy(buff,username,n);
    int n=strlen(password);
    if (n>19) {
        perror("limit exceeded");
    }
    memcpy(buff+20,username,n);
    buff[40]=auth;
    fwrite(buff,41,1,db1);
}

void addstudent(char* name,float s1,float s2,float s3,float s4,float s5) {
    char buff[41];
    bzero(buff,41);
    int n=strlen(name);
    if (n>19) {
        perror("limit exceeded");
    }
    memcpy(buff,name,n);

    short xs1=htons(round(s1*100));
    memcpy(buff+30,&xs1,2);
    short xs2=htons(round(s2*100));
    memcpy(buff+32,&xs2,2);
    short xs3=htons(round(s3*100));
    memcpy(buff+34,&xs1,2);
    short xs4=htons(round(s4*100));
    memcpy(buff+36,&xs1,2);
    short xs5=htons(round(s5*100));
    memcpy(buff+38,&xs1,2);
    char auth = 
    buff[40]=auth;
    fwrite(buff,41,1,db1);
}

void update(FILE* fp,int n,char change[]) {

}

void encrypt(char password[],int key){
    for (int i=0;i<strlen(password);i++) {
        password[i]=password[i]-key;
    }
}
void decrypt(char password[],int key){
    for (int i=0;i<strlen(password);i++) {
        password[i]=password[i]+key;
    }
}

void closing() {
    if (status&1) {close(sockfd);}
    if (status&2) {close(newsockfd);}
    if (status&4) {
        fclose(db1);
        fclose(db2);
    }

}

void error(const char *msg)
{
    perror(msg);
    closing();
    exit(1);
}

bool checker(char* username) {
    char* ptr;
    char x=0xff;
    write(newsockfd,&x,1);
    return true;
}

void connection() {
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, 
                (struct sockaddr *) &cli_addr, 
                &clilen);
    if (newsockfd < 0) 
        error("ERROR on accept");
    bzero(buffer,256);
    status=3;
}

void server() {
    bool run=true;
    char s;
    while (run) {
        switch (status) {
            case (2):
            connection();
            break;
            case (3):
            checker(buffer); // need to change
            break;
        }
        while ((s=getchar())!=EOF) {
            if (s=='e') run=false;
        }
    }
    closing();
    // close(newsockfd);
    // close(sockfd);
}

void userview() {
    int n;
    while (true) {
        n = read(newsockfd,buffer,255);
        if (n < 0) error("ERROR reading from socket");
        char* username = buffer;
        bool ch = checker(username);
        n = write(newsockfd,"I got your message",18);
        if (n < 0) error("ERROR writing to socket");
    }
}

int main(int argc, char *argv[])
{
    status=0;

    // Initializing Server
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    error("ERROR opening socket");
    status=1;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0) 
            error("ERROR on binding");
    listen(sockfd,5);
    printf("Server is ON\n");
    status|=2;  // need to check

    db1 = fopen("student_marks","a+b");
    if (db1==NULL) {
        perror("Error in loading database");
    }
    db2 = fopen("user_pass","a+b");
    if (db2==NULL){
        fclose(db1);
        perror("Error in loading database");
    }
    fseek(db1, 0L, SEEK_END);
    int res1 = ftell(db1);
    fseek(db1, 0L, SEEK_SET);
    fread(dbase1,res1,1,db1);
    fseek(db2, 0L, SEEK_END);
    int res2 = ftell(db2);
    fseek(db2, 0L, SEEK_SET);
    fread(dbase2,res2,1,db2);
    printf("Database is loaded\n");
    status|=4;

    server();
    
    return 0; 
}
/*
if (remove("abc.txt") == 0)
    printf("Deleted successfully");
else
    printf("Unable to delete the file");
*/

// int convert(unsigned char* up) {
//     int x=0;
//     x=(*up)>>4;
//     x*=10;
//     x+=((*up)&15);
//     up++;
//     x=(*up)>>4;
//     x*=10;
//     x+=((*up)&15);
//     return x;
// }
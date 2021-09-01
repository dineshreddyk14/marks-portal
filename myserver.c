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
int res1,res2;
char auth_cl;

FILE* db1; // marks
FILE* db2; // user pass

void closing() {
    if (status&1) {close(sockfd);}
    if (status&8) {close(newsockfd);}
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

void adduser(char* username,char* password,char auth) {
    char buff[41];
    bzero(buff,41);
    int n=strlen(username);
    if (n>19) {
        error("limit exceeded");
    }
    memcpy(buff,username,n);
    n=strlen(password);
    if (n>19) {
        error("limit exceeded");
    }
    memcpy(buff+20,password,n);
    buff[40]=auth;
    fwrite(buff,41,1,db2);
    memcpy(dbase2+res2,buff,41);
    res2+=41;
}

int addstudent(char* name,float s1,float s2,float s3,float s4,float s5) {
    char buff[41];
    bzero(buff,41);
    int n=strlen(name);
    if (n>19) {
        error("limit exceeded");
    }
    memcpy(buff,name,n);

    short xs=s1*100;
    xs=htons(xs);
    memcpy(buff+30,&xs,2);

    xs=s2*100;
    xs=htons(xs);
    memcpy(buff+32,&xs,2);

    xs=s3*100;
    xs=htons(xs);
    memcpy(buff+34,&xs,2);

    xs=s4*100;
    xs=htons(xs);
    memcpy(buff+36,&xs,2);

    xs=s5*100;
    xs=htons(xs);
    memcpy(buff+38,&xs,2);

    char auth = 1+(res1/41);
    buff[40]=auth;
    fwrite(buff,41,1,db1);
    memcpy(dbase1+res1,buff,41);
    res1+=41;
    return auth;
}

bool sendall() {
    printf("reached sendall");
    int x=htonl(res1);
    int n;
    n=write(newsockfd,&x,4);
    if (n<0){error("error in sendall");}
    int st=0;
    while (st<res1){
        n=write(newsockfd,dbase1,res1);
        if (n<0){error("error in sendall");}
        st+=n;
    }
    printf("completed send all");
    return true;
}

bool sendone(){
    printf("reached send one");
    int pos=(auth_cl-1)*41;
    int x=htonl(41l);

    int n;
    n=write(newsockfd,&x,4);
    if (n<0){error("error in sendone");}

    n=write(newsockfd,dbase1+pos,41);
    if (n<0){error("error in sendone");}

}

void update(int n,char change[]) {
    printf("update called for %d\n",n);
    short* ptr=(short*) change;
    int pos=30+ (n-1)*41;
    for (int i=0;i<10;i+=2) {
        fseek(db1,pos+i,SEEK_SET);
        printf("%d %d\n",pos+i,ntohs(*ptr));
        if (ntohs(*ptr)!=65535) {
            fwrite(ptr,2,1,db1);
            memcpy(dbase1,ptr,2);
            printf("update worked");
        }
        ptr++;
    }
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

void commands() {
    bzero(buffer,256);
    int n=read(newsockfd,buffer,255);
    if (n<0){
        error("error reading command");
    }
    if (n==0) {
        status&=7;
        close(newsockfd);
    }
    if(buffer[0]==-1){
        update(buffer[1],buffer+2);
    }
    else {
        close(newsockfd);
    }
}

int search(char* username) {
    int x=0;
    printf("search start");
    while (x<res2) {
        if (strcmp(dbase2+x,username)==0){
            return x;
        }
        x+=41;
    }
    return -1;
}

bool checker() {
    bzero(buffer,256);
    printf("yo waiting");
    int rnd=read(newsockfd,buffer,20);
    if (rnd<0){error("error while reading");return false;}
    printf("%s",buffer);
    int entry = search(buffer);
    if (entry<0) {
        write(newsockfd,&entry,4);
        status^=8;
        close(newsockfd);
        return false;
    }
    printf("search complete");
    char x=0xff;
    int key=11; //make a random number
    char pass[20];
    memcpy(pass,dbase2+entry+20,20);
    key&=(1<<24)-1;
    encrypt(pass,key);
    key=htonl(key);
    int snt = write(newsockfd,&key,4);
    if (snt<0){
        error("error on sending");
    }
    printf("send complete");
    fflush(stdout);
    bzero(buffer,256);
    int n = read(newsockfd,buffer,20);
    if (n < 0) error("ERROR reading from socket");
    if (strcmp(pass,buffer)!=0) {
        printf("%s %s",pass,buffer);
        n=-1;
        write(newsockfd,&n,4);
        status^=8;
        close(newsockfd);
        return false;
    }
    status|=16;
    auth_cl=dbase2[entry+40]; //if ch=-1 (int) ch == ?
    if ((auth_cl&(-16))==(-16)) {
        sendall();
    }
    else {
        sendone();
        close(newsockfd);
        status&=7;
    }
    return true;
}

void connection() {
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, 
                (struct sockaddr *) &cli_addr, 
                &clilen);
    if (newsockfd < 0) 
        error("ERROR on accept");
    status|=8;
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

    db1 = fopen("student_marks","r+b");
    if (db1==NULL) {
        error("Error in loading database");
    }
    db2 = fopen("user_pass","r+b");
    if (db2==NULL){
        fclose(db1);
        error("Error in loading database");
    }
    fseek(db1, 0L, SEEK_END);
    res1 = ftell(db1);
    fseek(db1, 0L, SEEK_SET);
    fread(dbase1,res1,1,db1);
    fseek(db2, 0L, SEEK_END);
    res2 = ftell(db2);
    fseek(db2, 0L, SEEK_SET);
    fread(dbase2,res2,1,db2);
    status|=4;
    printf("database is loaded\n");

    if (bind(sockfd, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0) 
            error("ERROR on binding");
    listen(sockfd,5);
    printf("Server is ON\n");
    status|=2;  // need to check


    // adduser("instructor2","adminpass2",-2);
    // addstudent("K Dinesh Reddy",99.05,98.61,90.86,99.99,96.30);
    int run=3;
    char s;
    while (run--) {
        if (!(status&8)) {
            printf("ready for connection\n");
            connection();
            printf("connection made\n");
        }
        if (status&8 && !(status&16)) {
            printf("ready for authentication\n");
            fflush(stdout);
            checker(); // need to change
            printf("authen happen\n");
        }
        if (status&16) {
            commands();
        }
        // if (getchar()=='e') {
        //     closing();
        //     return 0;
        // }
    }
    closing();
    return 0; 
}
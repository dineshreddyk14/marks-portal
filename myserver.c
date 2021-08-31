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

void commands() {
    "y";
}

int search(char* username) {
    int x=0;
    while (x<res2) {
        if (strcmp(dbase2+x,username)==0){
            return x;
        }
        x+=41;
    }
    return -1;
}

bool checker(char* username) {
    char* ptr;
    int n = search(username);
    if (n<0) {
        write(newsockfd,&n,4);
        status^=8;
        close(newsockfd);
        return false;
    }
    char x=0xff;
    int key; //make a random number
    char pass[20];
    memcpy(pass,dbase2+n+20,20);
    key&=(1<<24)-1;
    encrypt(pass,key);
    key=htonl(key);
    int snt = write(newsockfd,&key,4);
    if (snt<0){
        error("error on sending");
    }
    bzero(buffer,256);
    n = read(newsockfd,buffer,255);
    if (n < 0) error("ERROR reading from socket");
    if (strcmp(pass,buffer)!=0) {
        n=-1;
        write(newsockfd,&n,4);
        status^=8;
        close(newsockfd);
        return false;
    }
    status|=16;
    auth_cl=dbase2[n+40]; //if ch=-1 (int) ch == ?
    if (auth_cl&(-16)==(-16)) {
        sendall();
    }
    else {
        sendone();
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
    bzero(buffer,256);
    status|=8;
}

void server() {
    bool run=true;
    char s;
    while (run) {
        switch (status>>3) {
            case (0):
            connection();
            break;
            case (1):
            checker(buffer); // need to change
            break;
            case(3):
            commands();
            break;
            default:
            error("Something went wrong");
        }
        while ((s=getchar())!=EOF) {
            if (s=='e') run=false;
        }
    }
    closing();
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

    // adduser("instructor2","adminpass2",-2);
    // addstudent("K Dinesh Reddy",99.05,98.61,90.86,99.99,96.30);
    //server();
    closing();
    return 0; 
}
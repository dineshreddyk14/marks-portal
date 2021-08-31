#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

int res1,res2;

FILE* db1; // marks
FILE* db2; // user pass

void adduser(char* username,char* password,char auth) {
    char buff[41];
    bzero(buff,41);
    int n=strlen(username);
    if (n>19) {
        printf("limit exceeded");
    }
    memcpy(buff,username,n);
    n=strlen(password);
    if (n>19) {
        printf("limit exceeded");
    }
    memcpy(buff+20,password,n);
    buff[40]=auth;
    fwrite(buff,41,1,db2);
    // memcpy(dbase2+res2,buff,41);
    res2+=41;
}

int addstudent(char* name,short s1,short s2,short s3,short s4,short s5) {
    char buff[41];
    bzero(buff,41);
    int n=strlen(name);
    if (n>29) {
        printf("limit exceeded");
        exit(1);
    }
    memcpy(buff,name,n);
    short xs;
    xs=htons(s1);
    memcpy(buff+30,&xs,2);

    xs=htons(s2);
    memcpy(buff+32,&xs,2);

    xs=htons(s3);
    memcpy(buff+34,&xs,2);

    xs=htons(s4);
    memcpy(buff+36,&xs,2);

    xs=htons(s5);
    memcpy(buff+38,&xs,2);

    char auth = 1+(res1/41);
    buff[40]=auth;
    fwrite(buff,41,1,db1);
    // memcpy(dbase1+res1,buff,41);
    res1+=41;
    return auth;
}

int main() {
    
    srand(time(0));
    freopen("names.txt","r",stdin);
    db1 = fopen("student_marks","w+b");
    if (db1==NULL) {
        printf("Error in loading database");
    }
    db2 = fopen("user_pass","w+b");
    if (db2==NULL){
        fclose(db1);
        printf("Error in loading database");
    }
    fseek(db1, 0L, SEEK_END);
    res1 = ftell(db1);
    // fseek(db1, 0L, SEEK_SET);
    // fread(dbase1,res1,1,db1);
    fseek(db2, 0L, SEEK_END);
    res2 = ftell(db2);
    // fseek(db2, 0L, SEEK_SET);
    // fread(dbase2,res2,1,db2);
    adduser("instructor","adminpass",-1);
    adduser("instructor2","adminpass2",-2);
    char* name,*password,*username;
    short a[5];
    for (int i=0;i<24;i++) {
        sprintf(username,"student%d",i+1);
        sprintf(password,"password%d",i+1);
        gets(name);
        for (int j=0;j<5;j++){
            a[j]=(RAND_MAX-rand())%10000;
        }
        char auth= addstudent(name,a[0],a[1],a[2],a[3],a[4]);
        adduser(username,password,auth);
    }


    fclose(db1);
    fclose(db2);

}


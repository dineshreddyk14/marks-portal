#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;
char buffer[256];

char database[2000];
int dbsize=0;

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

void initialMenu();

void receiveAll(int k){
    printf("reached receive all");
    int st=0;
    while(st < k){
        printf("read start");
        if (st!=0 && st<k){printf("%d %d",st,k);return;}
        n = read(sockfd,database+st,2000);
        if(n < 0 ) error("ERROR reading database reply from socket");
        if (n==0) {printf("n==0");return;}
        st+=n;
        printf("read end");
    }
}

void studentlogin(){
    printf("Your available options:\n");
    printf("1. Marks in each subject\n2. Aggregate percentage\n3.Subjects with maximum and minimum marks\n4.Logout");
    printf("Enter suitable number: ");

    int cas;
    scanf("%d",&cas);

    if(cas==4){
        printf("Succesfully logged out\n");
        exit(0);
    }

    short markslist[5];
    for(int i=0;i<5;i++){
        markslist[i] = ntohs(*(short*)(database+(30+(2*i))));
    }

    if(cas == 1){
        printf("Your subject wise scores:\n");
        for(int i=0;i<5;i++){
            printf("Subject %d: %.2f\n",i+1,markslist[i]/100.0);
        }
    }else if(cas == 2){
        printf("Your aggregate score is ");
        int ag=0;
        for(int i=0;i<5;i++){
            ag+=markslist[i];
        }
        printf("%.2f\n",ag/500.0);
    }else if(cas == 3){
        int max=0;
        int min=0;
        for(int i=1;i<5;i++){
            if(markslist[i]>markslist[max])max=i;
            if(markslist[i]<markslist[min])min=i;
        }
        printf("Your maximum scoring subject is Subject %d\n",max+1);
        printf("You scored minimum in Subject %d\n",min+1);
    }
    studentlogin();
}

void instructorlogin(int k){
    printf("Your available options:\n");
    printf("1. Marks (individual and aggregate percentage) of each student\n2. Class average\n3. Number of students failed (passing percentage 33.33) in each subject\n4. Name of best and worst performing students\n5. Update student marks\n6. Logout");
    printf("Enter suitable number: ");
    int cas;
    scanf("%d",&cas);

    if(cas==6){
        printf("Succesfully logged out\n");
        exit(0);
    }

    int i=k/41; //no. of students

    short markslist[i][5];
    for(int j=0;j<i;j++){
        for(int k=0;k<5;k++){
            markslist[j][k] = ntohs(*(short*)(database+((41*j)+30+(2*k))));
        }
    }

    if(cas == 1){
        printf("Marks of all students are presented below in the order: subjects 1, 2, 3, 4, 5, aggregate percentage of all subjects and student name\n");
        for(int j=0;j<i;j++){
            int ag=0;
            for(int k=0;k<5;k++){
                printf("%.2f ",markslist[j][k]/100.0);
                ag+=markslist[j][k];
            }
            printf("%.2f %s\n",ag/500.0,database+j*41);
        }
    }else if(cas == 2){
        printf("Class average in each subject is presented\n");
        for(int k=0;k<5;k++){
            int av=0;
            for(int j=0;j<i;j++){
                av+=markslist[j][k];
            }
            printf("Subject %d: %.2f\n",k+1,av/(i*100.0));
        }
    }else if(cas == 3){
        printf("Number of students failed (passing percentage 33.33) in each subject is presented");
        for(int k=0;k<5;k++){
            int a=0;
            for(int j=0;j<i;j++){
                if(markslist[j][k]<3333)a++;
            }
            printf("Subject %d: %d\n",k+1,a);
        }
    }else if(cas == 4){
        int mn=0, mx=0;
        int sum=markslist[0][0]+markslist[0][1]+markslist[0][2]+markslist[0][3]+markslist[0][4];
        int summax=sum;
        int summin=sum;
        for(int j=1;j<i;j++){
            sum=0;
            for(int k=0;k<5;k++){
                sum+=markslist[j][k];
            }
            if(sum>summax){
                summax=sum;
                mx=j;
            }
            if(sum<summin){
                summin=sum;
                mn=j;
            }
        }
        printf("On aggregate of all subjects,\n");
        printf("Best performer of class is %s",database+(mx*41));
        printf("Last in the scoreboard is %s",database+(mn*41));
    }else if(cas == 5){
        printf("Select the student whose marks need to be updated\n");
        bzero(buffer,256);
        buffer[0]=-1;
        for(int j=0;j<i;j++){
            printf("%d. %s\n",j+1,database+j*41);
        }
        printf("Enter the student number: ");
        int st;
        scanf("%d",&st);
        if(st<1 || st>i)error("Invalid entry");
        buffer[1]=st;
        printf("Selected student name: %s",database+((st-1)*41));
        printf("Subjectwise marks of selected student in order subject 1, 2, 3, 4 and 5:\n");
        for(int k=0;k<5;k++){
            printf("%f\n",markslist[st-1][k]/100.0);
        }
        printf("Select the subject that need to be updated: ");
        int sb;
        scanf("%d",&sb);
        if(sb>5 || sb<1){
            error("Invalid entry");
        }
        for(int z=2;z<12;z++){buffer[z]=-1;}
        short* x = (short*)(buffer+sb*2);
        printf("Enter updated marks: ");
        float mk;
        scanf("%f",&mk);
        *x=htons((short)(mk*100));
        n = write(sockfd,buffer,12);
        if (n < 0) 
            error("ERROR writing updated marks to socket");
        printf("Succesfully updated!");
    }
    instructorlogin(k);
}

void checkpassword(int key){
    //Validating password

    //Sending encrypted password to server
    bzero(buffer,256);
    printf("Enter your password: \n");
    // getc(stdin);

    fgets(buffer,255,stdin);
    printf("read complete %syo",buffer);
    encrypt(buffer,key);
    n = write(sockfd,buffer,strlen(buffer)-1);
    if (n < 0) 
        error("ERROR writing password to socket");
    
    //Reading the reply from server
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
        error("ERROR reading password reply from socket");
    if(n == 0){
        error("Password incorrect\n");
    }

    //Logged in succesfully
    int k = ntohl(*(int*)buffer);

    receiveAll(k);
    printf("%d",k);
    if(k==41){
        printf("Succesfully logged in as a student\n");
        studentlogin();
    }else{
        printf("Succesfully logged in as an instructor\n");
        instructorlogin(k);
    }
}

void checkusername(){
    //Validating username

    //Sending username to server
    bzero(buffer,256);
    printf("Enter Username: \n");
    fgets(buffer,20,stdin);
    printf("read complete\nthis is it%s",buffer);
    n = write(sockfd,buffer,strlen(buffer)-1);
    if (n < 0) 
         error("ERROR writing username to socket");
    printf("write complete");
    //Reading the reply from server
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
        error("ERROR reading username reply from socket");
    printf("%s",buffer);
    int key = ntohl(*(int*)buffer);
    if(key==-1){
        error("Username doesn't exist");
    }
    checkpassword(key);
}

void initialMenu(){
    printf("1.Login\n2.Exit\nEnter either 1 or 2 :");
    int cas;
    printf("read start");

    scanf("%d",&cas);
    getc(stdin);
    printf("read complete\n");
    if(cas==2)exit(1); //exit
    checkusername();
}

int main(int argc, char *argv[])
{
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
    

    //Real processing starts here
    bzero(database,2000);
    //Initial menu
    initialMenu();

    close(sockfd);
    return 0;
}
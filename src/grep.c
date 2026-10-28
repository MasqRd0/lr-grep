#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdlib.h> 
#include<errno.h>


int min(int a,int b);
void reportError(char *filename,char *err);
int searchFile(char *filename,char *cnt);
int equal(char *p1,char *p2);
int printline(char *filename,int j);


int moden=0,modei=0,modec=0,needFileName=0;
int min(int a,int b){return (a>b) ? b:a;}
void reportError(char *filename,char *err)
{
    char *msg="lrg: ";
    write(STDERR_FILENO,msg,strlen(msg));
    write(STDERR_FILENO,filename,strlen(filename));
    write(STDERR_FILENO,": ",2);
    write(STDERR_FILENO,err,strlen(err));
    write(STDERR_FILENO,"\n",1);
}
int equal(char *p1,char *p2)
{
    if(modei==0) return (*p1==*p2);
	if(modei==1)
	{
		if((*p1<=122&&*p1>=97&&*p2<=90&&*p2>=65)||(*p2<=122&&*p2>=97&&*p1<=90&&*p1>=65))
        {    
            if(abs(*p1-*p2)==32||*p1==*p2) return 1;
            else return 0;
        }
        else return (*p1==*p2);
	}
    return 0;
}
int printline(char *filename,int j)
{
    char *c=malloc(sizeof(char));
    *c='\n';
    int fd=open(filename,O_RDONLY,0);
    if(fd==-1){free(c);reportError(filename,strerror(errno));needFileName--;return 2;}
    int y=1,head=1,readsuccess=0;
    while(y<=j)
    {
        if((readsuccess=read(fd,c,1))==1)
        {
            if(y==j)
            {
                if(head==1)
                {
                    if(needFileName>1) printf("%s:",filename);
                    if(moden==1) printf("%d:",y);
                    head=0;
                }
                fflush(stdout);
                write(STDOUT_FILENO,c,1);
            }
            if(*c=='\n') y++,head=1;
        }
        else if(readsuccess==0&&*c!='\n')
        {    
            printf("\n");
            break;
        }
        else if(readsuccess==-1&&errno==EINTR) continue;
        else
        {
            reportError(filename,strerror(errno));
            close(fd);
            free(c);
            return 2;
        }
    }
    close(fd);
    free(c);
    return 0;
}
int searchFile(char *filename,char *cnt)
{
    int i=0,j=1,n=0,readsuccess=0,count=0,hasprint=0,matchline=0;
	char *ch=malloc(sizeof(char));
	int fd=open(filename,O_RDONLY,0);
    if(fd==-1){free(ch);reportError(filename,strerror(errno));needFileName--;return 2;}
    int m=strlen(cnt);
    int *border=malloc(m*sizeof(int));
    border[0]=0;
    for(int k=1;k<m;k++)
    {
        int t=border[k-1];
        while(t>0&&!equal(&cnt[k],&cnt[t])) t=border[t-1];
        if(equal(&cnt[k],&cnt[t])) t++;
        border[k]=t;
    }
	while(1)
	{	
		if((readsuccess=read(fd,ch,1))==1)
		{
			while(1)
            {
            if(i<m&&equal(ch,&cnt[i])) {i++;break;}
            if(i==0) break;
            i=border[i-1];     
            }
            if(i==m)
            {
                i=border[m-1];
                if(modec==0&&hasprint==0)
                {
                    if(printline(filename,j)==2){free(ch);return 2;}
                    hasprint=1;
                }
                count++;
            }
            if(*ch=='\n')
            {
                j++;
                hasprint=0;
                matchline+=(count>0) ? 1:0;
                count=0;
            }
		}
        else if(readsuccess==-1&&errno==EINTR) continue;
        else if(readsuccess==0) break;
        else
        {
            reportError(filename,strerror(errno));
            close(fd);
            free(ch);
            free(border);
            return 2;
        }
	}
    close(fd);
    matchline+=(count>0) ? 1:0;
    if(modec==1)
    {
        if(needFileName>1) printf("%s:",filename);
        printf("%d\n",matchline);
    }
    free(ch);
    free(border);
    if(matchline==0) return 1;
    return 0;
}
int main(int argc,char *argv[])
{
    int inputMode=1,inputPattern=1,inputFileName=1,argnum=argc-1,returnCode=1,test=0;
    char *str=NULL;
    char *filename=NULL;
    for(int i=1;i<argc;i++)
    {
//        if(argv[i]==NULL) continue;
        if(argv[i][0]=='-'&&inputMode==1)
        {
            for(int j=1;j<strlen(argv[i]);j++)
            {
                if(argv[i][j]=='n') moden=1;
                else if(argv[i][j]=='i') modei=1;
                else if(argv[i][j]=='c') modec=1,moden=0;
                else
                {
                    char *msg="usage: lrg [-nic] PATTERN [FILE...]\n";
                    write(STDERR_FILENO,msg,strlen(msg));
                    return 2;
                }
                if(modec) moden=0;
            }
            argnum--;
            continue;
        }
        else inputMode=0;
        if(inputPattern)
        {
            str=argv[i];
            inputPattern=0;
            argnum--;
            continue;
        }
        if(inputFileName)
        {
            if(argnum>=2) needFileName=argnum;
            filename=argv[i];
/*
            if(test==0)
                for(int j=i;j<argc;j++)
                {
                    int fd=open(argv[j],O_RDONLY,0);
                    if(fd==-1)
                    {
                        reportError(argv[j],strerror(errno));
                        argv[j]=NULL;
                        needFileName--;
                        returnCode=2;
                    }
                    else close(fd);
                }
            test=1;
            if(argv[i]==NULL) continue;//防止第一个文件无法打开导致报错两次
*/
            int sr=searchFile(filename,str);
            if(returnCode!=2) returnCode=(sr>1) ? 2:min(returnCode,sr); 
            continue;
        }
        
    }
    if(str==NULL)
    {
        char *msg="usage: lrg [-nic] PATTERN [FILE...]\n";
        write(STDERR_FILENO,msg,strlen(msg));
        return 2;
    } 
    if(filename==NULL)
    {
        int m=strlen(str);
        int *border=malloc(m*sizeof(int));
        border[0]=0;
        for(int k=1;k<m;k++)
        {
            int t=border[k-1];
            while(t>0&&!equal(&str[k],&str[t])) t=border[t-1];
            if(equal(&str[k],&str[t])) t++;
            border[k]=t;
        }        
        char *ch=malloc(sizeof(char));
        size_t cap=128*sizeof(char);
        char *buf=malloc(cap);
        int count=0,readsuccess=1,i=0,j=1,word=0,matchline=0;
        if(ch==NULL) return -1;
        if(buf==NULL) return -1;
        while(1)
        {
            if((readsuccess=read(STDIN_FILENO,ch,1))==1)
            {
                buf[word++]=*ch;
                if(word+1>=cap)
                {
                    cap*=2;
                    char *p=realloc(buf,cap);
                    if(p==NULL) return -1;
                    buf=p; 
                }
                while(1)
                {
                    if(i<m&&equal(ch,&str[i])) {i++;break;}
                    if(i==0) break;
                    i=border[i-1];     
                }
                if(i==m)
                {
                    i=border[m-1];
                    count++;
                }
                if(*ch=='\n')
                {
                    buf[word]='\0';
                    if(modec==0&&moden&&count) printf("%d:",j);
                    if(modec==0&&count) printf("%s",buf);
                    matchline+=(count>0) ? 1:0;
                    j++;
                    count=0;
                    word=0;                
                }
            }
            else if(readsuccess==-1&&errno==EINTR) continue;
            else if(readsuccess==0) break;
            else
            {
            reportError("STDIN",strerror(errno));
            free(buf);
            free(ch);
            return 2;
            }
        }
        if(word>0)                       
        {
            buf[word]='\0';
            if(modec==0&&count)
            {
                if(moden) printf("%d:",j);
            printf("%s",buf);
            if(buf[word-1]!='\n') printf("\n");
            }
        }
        matchline+=(count>0) ? 1:0;
        if(matchline!=0) returnCode=0;
        if(modec) printf("%d\n",matchline);
        free(buf);
        free(ch);
        free(border);
    }
    return returnCode;
}



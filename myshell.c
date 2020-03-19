#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<fcntl.h>
#include<signal.h>
#include<ctype.h>
#include<dirent.h>


#define MAX_CMD_ARG 10
#define MAX_CMD_ZIP 10
#define BUFSIZE 256

const char *prompt="YuniShell> ";
char* cmdvector[MAX_CMD_ARG];
char cmdline[BUFSIZE];
char* cmdzip[MAX_CMD_ZIP];
char* cmdpipe[MAX_CMD_ARG];
int background;

void m_cd(int argc,char** argv);
void fatal(char *str);
void execute_cmdline(char *cmd);
void execute_cmdzip(char* cmdzip);
int makelist(char *s,const char *delimiters,char** list, int MAX_LIST);
void zombie_handler();
void redirection(char *cmdline);
void execute_pipe(char *cmd);

void fatal(char *str){
	perror(str);
	exit(1);
}


//입력받은 command line을 exevp로  실행하기 위해 vector 형태로 바꿔주는 함수. 
int makelist(char *s,const char *delimiters, char** list, int MAX_LIST){
	int i=0;
	int numtokens=0;
	char *snew=NULL;

	if((s==NULL)||(delimiters==NULL)) return -1;

	snew=s+strspn(s,delimiters); 

	if((list[numtokens]=strtok(snew,delimiters))==NULL)
		return numtokens;

	if(list[numtokens]!=NULL){
		for(numtokens=1;(list[numtokens]=strtok(NULL,delimiters))!=NULL;numtokens++){
			if(numtokens==(MAX_LIST-1)) return -1;
		}
	}
	
	if(numtokens>MAX_LIST) return -1;
	
	return numtokens;
}

void execute_cmdline(char* cmdline){
	int count=0;
	int m_cd_;
	int i=0;
	int j,pid;
	count=makelist(cmdline,";",cmdzip,MAX_CMD_ZIP);

	for(i=0;i<count;++i){
		background =0;
		for(j=0;j<strlen(cmdzip[i]);j++){
			if(cmdzip[i][j]=='&'){
				background=1;
				cmdzip[i][j]='\0';
				break;
			}
		}

		if(cmdzip[i][0]=='c'&&cmdzip[i][1]=='d'){
			m_cd_=makelist(cmdzip[i]," ",cmdvector,MAX_CMD_ARG);
			m_cd(m_cd_,cmdvector);
		}
		else if(strcmp(cmdzip[i],"exit")==0){
			printf("exit.\n");
			exit(1);
		}
		else{
			switch(pid=fork()){
				case -1:
					fatal("fork error");
				case 0:
					signal(SIGQUIT,SIG_DFL);
					signal(SIGINT,SIG_DFL);
					setpgid(0,0);
					if(!background)
						tcsetpgrp(STDIN_FILENO,getpgid(0));
					execute_pipe(cmdzip[i]);
					break;

				default:
					if(background) break;
					if(!background){
						waitpid(pid,NULL,0);
						tcsetpgrp(STDIN_FILENO,getpgid(0));
					}
					fflush(stdout);
			}
		}
	}
}

void execute_pipe(char *cmd){
	int p[2];
	int count;
	int i;

	count =makelist(cmd,"|",cmdpipe,MAX_CMD_ARG);

	for(i=0;i<count-1;i++){
		pipe(p);

		switch(fork()){
			case -1:
				fatal("fork error");
			case 0:
				close(p[0]);
				dup2(p[1],1);
				execute_cmdzip(cmdpipe[i]);

			default:
				close(p[1]);
				dup2(p[0],0);
		}
	}
	execute_cmdzip(cmdpipe[i]);
}

void redirection(char *cmdline){
	char *filename;
	int fd;
	int i;
	int length=strlen(cmdline);

	for(i=0;i<length;i++){
		if(cmdline[i]=='<'){
			filename=strtok(&cmdline[i+1]," ");
			fd=open(filename,O_RDONLY|O_CREAT,0777);
			dup2(fd,0);
			cmdline[i]='\0';
		}

		if(cmdline[i]=='>'){
			filename=strtok(&cmdline[i+1]," ");
			fd=open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777);
			dup2(fd,1);
			cmdline[i]='\0';
		}
	}
}

void execute_cmdzip(char *cmdzip){
	int count=0;

	redirection(cmdzip);

	count=makelist(cmdzip," ",cmdvector,MAX_CMD_ARG);
	execvp(cmdvector[0],cmdvector);

	fatal("exec error");
}

void zombie_handler(){
	waitpid(-1,NULL,WNOHANG);
}



void  m_cd(int argc,char** argv){
	int cur;

	if(argc==1){
		cur=chdir(getenv("HOME")); 
	       	//cd 뒤에 특정한 dir없이  cd만 입력된경우 home dir로 이동
		if(cur==-1)
			printf("error\n");
	}

	else if(argc==2){
		cur=chdir(argv[1]); //cd 뒤에 특정한 dir가 있다면 거기로 이동
		if(cur==-1)
			printf("No directory\n");
	}

	else
		printf("error\n");
}


int main(int argc,char** argv){
	int i=0;
	int type=0;
	int length=0;
	struct sigaction m_act;

	memset(&m_act,0,sizeof(m_act));
	m_act.sa_handler=zombie_handler;
	m_act.sa_flags=SA_RESTART;
	sigaction(SIGCHLD,&m_act,NULL);

	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);

	while(1){
		fputs(prompt,stdout);
		fgets(cmdline,BUFSIZE,stdin);
		cmdline[strlen(cmdline)-1]='\0';
		execute_cmdline(cmdline);
		printf("\n");
	}

	return 0;
}


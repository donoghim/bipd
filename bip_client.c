#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#define BUF_SIZE 1024

void error_handling(char *message);

int main(int argc, char *argv[]){
	int sock;
	// char message[BUF_SIZE];
	char txdata[BUF_SIZE];
	char rxdata[BUF_SIZE];
	int str_len;
	int inx;
	struct sockaddr_in serv_adr;
	
	if(argc!=3){
		printf("Usage : %s <IP> <port>\n",argv[0]);
		exit(1);
	}
	sock=socket(PF_INET,SOCK_STREAM,0);
	if(sock==-1)
		error_handling("socket() error");
	
	memset(&serv_adr,0,sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error");
	else
		puts("Connected..........");
	
	while(1){
		fputs("Input data(Q to quit): ", stdout);
		fgets(txdata, BUF_SIZE, stdin);
		if(!strcmp(txdata,"q\n") || !strcmp(txdata,"Q\n"))
			break;
		write(sock,txdata, strlen(txdata));

    memset(rxdata, 0x00, BUF_SIZE);
		str_len=read(sock, rxdata, BUF_SIZE-1);
		printf("recv data len:%d\n", str_len);
    for(inx=0; inx<str_len; inx++)
    {
      printf("%02X ", rxdata[inx]);
    }
    printf("str:%s ", rxdata);
		printf("\n");
	}

	close(sock);
	return 0;
}

void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n',stderr);
	exit(1);
}

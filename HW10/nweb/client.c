//client.c Client demo for nweb, 'A Tiny, Safe Webserver' by Nigel Griffiths
//	-compiles on WSL with: "$ gcc -Wall  client.c -lresolv -o client.exe " 
//	-runs on WSL with  "$ ./client
//!!note: PORT and IP_ADDRESS below must match those which nweb was started with

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//hardcoded server IP (too lazy to do it better) 
#define PORT        8182			/* port number is an int */
#define IP_ADDRESS "127.0.0.1"	/* IP address is a string, discovered with '(glue)% ifconfig -a' */

#define BUFSIZE 8196

pexit(char * msg)
{
	perror(msg);
	exit(1);
}

main()
{
int i,sockfd;
char buffer[BUFSIZE];
static struct sockaddr_in serv_addr;

	printf("client trying to connect to %s and port %d\n",IP_ADDRESS,PORT);
	if((sockfd = socket(AF_INET, SOCK_STREAM,0)) <0) 
		pexit("socket() failed");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	serv_addr.sin_port = htons(PORT);

	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <0)
		pexit("connect() failed");

	/* now the sockfd can be used to communicate to the server */
	write(sockfd, "GET /index.html \r\n", 18);
	/* note second space is a delimiter and important */

	/* this displays the raw HTML file as received by the browser */
	while( (i=read(sockfd,buffer,BUFSIZE)) > 0)
		write(1,buffer,i);
}

#include "ism43362_socket.h"   /* provides socket, connect, write, read,
                                  close, sockaddr_in, htons, inet_addr,
                                  AF_INET, SOCK_STREAM                   */
#include "ism43362_config.h"   /* ISM_SERVER_IP, ISM_SERVER_PORT         */

#include <stdio.h>
#include <stdlib.h>

#define PORT        ISM_SERVER_PORT
#define IP_ADDRESS  ISM_SERVER_IP

#define BUFSIZE 8196

static void pexit(const char *msg)
{
    printf("FATAL: %s  [%s]\r\n", msg, ism_last_error());
    for (;;) {}
}

void ism_client_main(void)
{
    int i, sockfd;
    char buffer[BUFSIZE];
    static struct sockaddr_in serv_addr;

    /* ---- bring up Wi-Fi before any socket call ---- */
    printf("Initialising Wi-Fi...\r\n");
    if (ism_wifi_init() != 0)
        pexit("ism_wifi_init() failed");
    printf("Joined hotspot.  Connecting to nweb at %s:%d\r\n",
           IP_ADDRESS, PORT);

    printf("client trying to connect to %s and port %d\n", IP_ADDRESS, PORT);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        pexit("socket() failed");

    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    serv_addr.sin_port        = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        pexit("connect() failed");

    /* now the sockfd can be used to communicate to the server */
    write(sockfd, "GET /index.html \r\n", 18);

    /* this displays the raw HTML file as received by the browser */
    while ((i = read(sockfd, buffer, BUFSIZE)) > 0)
        write(1, buffer, i);

    close(sockfd);
    printf("\r\nDone.\r\n");
}

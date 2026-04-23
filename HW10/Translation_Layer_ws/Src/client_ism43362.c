/**
 * client_ism43362.c
 *
 * client.c adapted for the B-L475E-IOT01A (ISM43362 Wi-Fi module),
 * targeting the STM32CubeIDE Wifi_Client_Server example project.
 *
 * Changes from the original client.c
 * =====================================
 * 1. Linux socket/network headers replaced by ism43362_socket.h.
 *
 * 2. PORT and IP_ADDRESS are taken from ism43362_config.h rather than
 *    being hardcoded here – edit that file to point at your PC.
 *
 * 3. main() renamed to ism_client_main() so it can be called from the
 *    CubeIDE project's main.c without a name conflict.
 *
 * 4. ism_wifi_init() called once at the top to join the hotspot before
 *    any socket operation.
 *
 * 5. pexit() uses printf + infinite loop instead of perror + exit(),
 *    which are unsafe on bare-metal.
 *
 * Everything else is identical to the original client.c so the diff
 * stays minimal and easy to verify.
 *
 * How to call from main.c
 * =======================
 * Declare the function and call it inside the USER CODE blocks so that
 * CubeMX code regeneration does not erase it:
 *
 *   // In main.c, before the while(1) loop:
 *   // USER CODE BEGIN 2
 *   extern void ism_client_main(void);
 *   ism_client_main();
 *   // USER CODE END 2
 */

#include "ism43362_socket.h"   /* provides socket, connect, write, read,
                                  close, sockaddr_in, htons, inet_addr,
                                  AF_INET, SOCK_STREAM                   */
#include "ism43362_config.h"   /* ISM_SERVER_IP, ISM_SERVER_PORT         */

#include <stdio.h>
#include <stdlib.h>

#define PORT        ISM_SERVER_PORT
#define IP_ADDRESS  ISM_SERVER_IP

#define BUFSIZE 8196

/* On bare-metal there is no OS to return to, so spin instead of exit(). */
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

    /* ---- from here down this is verbatim client.c ---- */

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
    /* note second space is a delimiter and important */

    /* this displays the raw HTML file as received by the browser */
    while ((i = read(sockfd, buffer, BUFSIZE)) > 0)
        write(1, buffer, i);

    /* ---- tidy up (original did not bother, but good practice) ---- */
    close(sockfd);
    printf("\r\nDone.\r\n");
}

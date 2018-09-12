#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>
#include <stdint.h>

#include "http_request.h"
#include "http_response.h"
#include "http_router.h"

#include "esp_log.h"

static const char * TAG = "HTTPD";

static void pong(Request *request, Response *response)
{
  ESP_LOGI(TAG, "request: ping");
  response_write(response, (uint8_t *)"pong");
}

static int sockfd, newsockfd, portno;
static struct sockaddr_in serv_addr, cli_addr;
static unsigned int clilen;
static char buffer[256];
static Router *router;

void ringthing_http_start_server()
{
  ESP_LOGI(TAG, "initialize http server");

  /* First call to socket() function */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0)
  {
    perror("ERROR opening socket");
    exit(1);
  }

  /* Initialize socket structure */
  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = 5001;

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* Now bind the host address using bind() call.*/
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("ERROR on binding");
    exit(1);
  }

  router = router_create();
  router_add_route(router, (uint8_t *)"/ping", pong);

  /* Now start listening for the clients, here process will
  * go in sleep mode and will wait for the incoming connection
  */
  listen(sockfd, 500);
  clilen = sizeof(cli_addr);
}

void ringthing_http_server_loop()
{
  ESP_LOGI(TAG, "execute http worker loop");
  int n;
  /* Accept actual connection from the client */
  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

  if (newsockfd < 0)
  {
    perror("ERROR on accept");
  }

  /* If connection is established then start communicating */
  bzero(buffer, 256);
  n = read(newsockfd, buffer, 255);

  if (n < 0)
  {
    perror("ERROR reading from socket");
    close(newsockfd);
  }

  route((uint8_t)newsockfd, router, (uint8_t *)buffer);
  close(newsockfd);
}
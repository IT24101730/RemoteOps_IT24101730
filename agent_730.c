#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define AGENT_PORT 9410
#define BACKLOG 5

int main(void)
{
    int server_socket;
    int client_socket;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    socklen_t client_address_length;

    /* Create a TCP socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        perror("socket");
        return 1;
    }

    /* Prepare the server address */
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(AGENT_PORT);

    /* Bind the socket to port 9410 */
    if (bind(server_socket,
             (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0)
    {
        perror("bind");
        close(server_socket);
        return 1;
    }

    /* Listen for incoming connections */
    if (listen(server_socket, BACKLOG) < 0)
    {
        perror("listen");
        close(server_socket);
        return 1;
    }

    printf("RemoteOps Agent listening on TCP port %d\n", AGENT_PORT);

    /* Wait for one Controller connection */
    client_address_length = sizeof(client_address);

    client_socket = accept(server_socket,
                           (struct sockaddr *)&client_address,
                           &client_address_length);

    if (client_socket < 0)
    {
        perror("accept");
        close(server_socket);
        return 1;
    }

    printf("Controller connected from %s\n",
           inet_ntoa(client_address.sin_addr));

    /* Close the client connection */
    close(client_socket);

    /* Close the listening socket */
    close(server_socket);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define AGENT_PORT 9410
#define AGENT_IP "127.0.0.1"

int main(void)
{
    int controller_socket;
    struct sockaddr_in agent_address;

    /* Create a TCP socket */
    controller_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (controller_socket < 0)
    {
        perror("socket");
        return 1;
    }

    /* Prepare the Agent address */
    memset(&agent_address, 0, sizeof(agent_address));

    agent_address.sin_family = AF_INET;
    agent_address.sin_port = htons(AGENT_PORT);

    if (inet_pton(AF_INET, AGENT_IP, &agent_address.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(controller_socket);
        return 1;
    }

    /* Connect to the Agent */
    if (connect(controller_socket,
                (struct sockaddr *)&agent_address,
                sizeof(agent_address)) < 0)
    {
        perror("connect");
        close(controller_socket);
        return 1;
    }

    printf("Connected to RemoteOps Agent at %s:%d\n",
           AGENT_IP, AGENT_PORT);

    /* Close the connection */
    close(controller_socket);

    return 0;
}

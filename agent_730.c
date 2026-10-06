#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define AGENT_PORT 9410
#define BACKLOG 5
#define RECEIVE_BUFFER_SIZE 4096
#define LINE_SIZE 2048

/*
 * Keeps TCP data that has been received but
 * has not yet been returned as a complete line.
 */
typedef struct
{
    char data[RECEIVE_BUFFER_SIZE];
    size_t used;
} LineReader;


/*
 * recv_line()
 *
 * Return values:
 *  1  = complete line received
 *  0  = client disconnected cleanly
 * -1  = recv() error
 * -2  = line is too long
 * -3  = client disconnected with an incomplete line
 */
int recv_line(int socket_fd,
              LineReader *reader,
              char *line,
              size_t line_size)
{
    while (1)
    {
        size_t i;

        /* Look for a newline in data already received */
        for (i = 0; i < reader->used; i++)
        {
            if (reader->data[i] == '\n')
            {
                size_t line_length = i;

                if (line_length >= line_size)
                {
                    return -2;
                }

                memcpy(line, reader->data, line_length);
                line[line_length] = '\0';

                /*
                 * Remove optional carriage return.
                 * This allows "\r\n" as well as "\n".
                 */
                if (line_length > 0 &&
                    line[line_length - 1] == '\r')
                {
                    line[line_length - 1] = '\0';
                }

                /*
                 * Keep any bytes after this newline.
                 * They may contain another complete line.
                 */
                memmove(reader->data,
                        reader->data + i + 1,
                        reader->used - (i + 1));

                reader->used -= (i + 1);

                return 1;
            }
        }

        /*
         * No newline was found.
         * Make sure the receive buffer is not full.
         */
        if (reader->used == sizeof(reader->data))
        {
            return -2;
        }

        /*
         * Receive more TCP bytes and append them
         * after the bytes already stored.
         */
        ssize_t bytes_received = recv(
            socket_fd,
            reader->data + reader->used,
            sizeof(reader->data) - reader->used,
            0);

        if (bytes_received > 0)
        {
            reader->used += (size_t)bytes_received;
            continue;
        }

        if (bytes_received == 0)
        {
            /*
             * Client closed the connection.
             */
            if (reader->used == 0)
            {
                return 0;
            }

            /*
             * Data existed but there was no final '\n'.
             */
            return -3;
        }

        /*
         * If recv() was interrupted, try again.
         */
        if (errno == EINTR)
        {
            continue;
        }

        return -1;
    }
}


int main(void)
{
    int server_socket;
    int client_socket;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    socklen_t client_address_length;

    LineReader reader = { .used = 0 };

    char line[LINE_SIZE];

    /* Create TCP socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        perror("socket");
        return 1;
    }

    /* Prepare Agent address */
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(AGENT_PORT);

    /* Bind Agent to personalised port 9410 */
    if (bind(server_socket,
             (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0)
    {
        perror("bind");
        close(server_socket);
        return 1;
    }

    /* Listen for Controller connections */
    if (listen(server_socket, BACKLOG) < 0)
    {
        perror("listen");
        close(server_socket);
        return 1;
    }

    printf("RemoteOps Agent listening on TCP port %d\n",
           AGENT_PORT);

    client_address_length = sizeof(client_address);

    /* Accept one Controller for this development step */
    client_socket = accept(
        server_socket,
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

    /*
     * Read complete newline-terminated commands.
     */
    while (1)
    {
        int result = recv_line(
            client_socket,
            &reader,
            line,
            sizeof(line));

        if (result == 1)
        {
            printf("Received line: [%s]\n", line);
        }
        else if (result == 0)
        {
            printf("Controller disconnected\n");
            break;
        }
        else if (result == -2)
        {
            printf("Received line is too long\n");
            break;
        }
        else if (result == -3)
        {
            printf("Controller disconnected with incomplete line\n");
            break;
        }
        else
        {
            perror("recv");
            break;
        }
    }

    close(client_socket);
    close(server_socket);

    return 0;
}

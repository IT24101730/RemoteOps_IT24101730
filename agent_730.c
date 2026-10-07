#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/* Personalised values */
#define AGENT_PORT 9410
#define AUTH_TOKEN "OPS-1730"
#define SESSION_ID "0371"

/* Server settings */
#define BACKLOG 5
#define RECEIVE_BUFFER_SIZE 4096
#define LINE_SIZE 2048


/*
 * Stores TCP bytes that have been received
 * but have not yet formed a complete line.
 */
typedef struct
{
    char data[RECEIVE_BUFFER_SIZE];
    size_t used;
} LineReader;


/*
 * recv_line()
 *
 * Reads exactly one newline-terminated text line
 * from the TCP byte stream.
 *
 * Return values:
 *  1  = complete line received
 *  0  = client disconnected normally
 * -1  = recv() error
 * -2  = line too long
 * -3  = disconnected with incomplete line
 */
int recv_line(int socket_fd,
              LineReader *reader,
              char *line,
              size_t line_size)
{
    while (1)
    {
        size_t i;

        /*
         * Search already-received data
         * for a newline.
         */
        for (i = 0; i < reader->used; i++)
        {
            if (reader->data[i] == '\n')
            {
                size_t line_length = i;

                if (line_length >= line_size)
                {
                    return -2;
                }

                /*
                 * Copy one complete line.
                 */
                memcpy(line,
                       reader->data,
                       line_length);

                line[line_length] = '\0';

                /*
                 * Remove optional '\r'
                 * when input uses "\r\n".
                 */
                if (line_length > 0 &&
                    line[line_length - 1] == '\r')
                {
                    line[line_length - 1] = '\0';
                }

                /*
                 * Preserve bytes after this newline.
                 * They may contain another command.
                 */
                memmove(reader->data,
                        reader->data + i + 1,
                        reader->used - (i + 1));

                reader->used -= (i + 1);

                return 1;
            }
        }

        /*
         * Buffer is full but no newline appeared.
         */
        if (reader->used == sizeof(reader->data))
        {
            return -2;
        }

        /*
         * Receive more TCP bytes.
         */
        ssize_t bytes_received =
            recv(socket_fd,
                 reader->data + reader->used,
                 sizeof(reader->data) - reader->used,
                 0);

        if (bytes_received > 0)
        {
            reader->used += (size_t)bytes_received;
            continue;
        }

        /*
         * recv() returning 0 means
         * the Controller disconnected.
         */
        if (bytes_received == 0)
        {
            if (reader->used == 0)
            {
                return 0;
            }

            /*
             * Connection closed while a partial
             * line was still in the buffer.
             */
            return -3;
        }

        /*
         * If recv() was interrupted,
         * try again.
         */
        if (errno == EINTR)
        {
            continue;
        }

        return -1;
    }
}


/*
 * send_all()
 *
 * Ensures that all bytes are sent even if
 * send() writes only part of the buffer.
 */
int send_all(int socket_fd,
             const char *buffer,
             size_t length)
{
    size_t total_sent = 0;

    while (total_sent < length)
    {
        ssize_t bytes_sent =
            send(socket_fd,
                 buffer + total_sent,
                 length - total_sent,
                 0);

        if (bytes_sent > 0)
        {
            total_sent += (size_t)bytes_sent;
            continue;
        }

        if (bytes_sent < 0 && errno == EINTR)
        {
            continue;
        }

        return -1;
    }

    return 0;
}


/*
 * send_response()
 *
 * Sends one RemoteOps protocol response.
 * Automatically adds the personalised SID
 * and the required newline.
 */
int send_response(int socket_fd,
                  const char *message)
{
    char response[LINE_SIZE];

    int length =
        snprintf(response,
                 sizeof(response),
                 "%s SID:%s\n",
                 message,
                 SESSION_ID);

    if (length < 0 ||
        (size_t)length >= sizeof(response))
    {
        return -1;
    }

    return send_all(socket_fd,
                    response,
                    (size_t)length);
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

    /*
     * Each Controller session begins
     * unauthenticated.
     */
    int authenticated = 0;


    /*
     * Create IPv4 TCP socket.
     */
    server_socket =
        socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        perror("socket");
        return 1;
    }


    /*
     * Prepare Agent address.
     */
    memset(&server_address,
           0,
           sizeof(server_address));

    server_address.sin_family = AF_INET;

    server_address.sin_addr.s_addr =
        INADDR_ANY;

    server_address.sin_port =
        htons(AGENT_PORT);


    /*
     * Bind the Agent to personalised
     * TCP port 9410.
     */
    if (bind(server_socket,
             (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0)
    {
        perror("bind");

        close(server_socket);

        return 1;
    }


    /*
     * Start listening for Controllers.
     */
    if (listen(server_socket,
               BACKLOG) < 0)
    {
        perror("listen");

        close(server_socket);

        return 1;
    }


    printf("RemoteOps Agent listening on TCP port %d\n",
           AGENT_PORT);


    /*
     * Accept one Controller.
     *
     * Multiple simultaneous Controllers
     * will be added in the concurrency step.
     */
    client_address_length =
        sizeof(client_address);

    client_socket =
        accept(server_socket,
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
     * Process newline-terminated commands.
     */
    while (1)
    {
        int result =
            recv_line(client_socket,
                      &reader,
                      line,
                      sizeof(line));


        if (result == 1)
        {
            printf("Received line: [%s]\n",
                   line);


            /*
             * AUTH command
             */
            if (strncmp(line,
                        "AUTH ",
                        5) == 0)
            {
                const char *token =
                    line + 5;


                if (strcmp(token,
                           AUTH_TOKEN) == 0)
                {
                    if (send_response(
                            client_socket,
                            "OK AUTHENTICATED") < 0)
                    {
                        perror("send");
                        break;
                    }

                    authenticated = 1;

                    printf(
                        "Controller authenticated successfully\n");
                }
                else
                {
                    if (send_response(
                            client_socket,
                            "ERR 001 AUTH_FAILED") < 0)
                    {
                        perror("send");
                        break;
                    }

                    printf(
                        "Controller authentication failed\n");
                }

                continue;
            }


            /*
             * No other command is accepted
             * before authentication.
             */
            if (!authenticated)
            {
                if (send_response(
                        client_socket,
                        "ERR 003 AUTH_REQUIRED") < 0)
                {
                    perror("send");
                    break;
                }

                continue;
            }


            /*
             * Other authenticated commands
             * will be implemented in later steps.
             */
            if (send_response(
                    client_socket,
                    "ERR 006 UNKNOWN_COMMAND") < 0)
            {
                perror("send");
                break;
            }
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
            printf(
                "Controller disconnected with incomplete line\n");

            break;
        }


        else
        {
            perror("recv");
            break;
        }
    }


    /*
     * Close sockets.
     */
    close(client_socket);
    close(server_socket);

    return 0;
}

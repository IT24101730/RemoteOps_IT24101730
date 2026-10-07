#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define AGENT_PORT 9410
#define AGENT_IP "127.0.0.1"

#define BUFFER_SIZE 4096
#define LINE_SIZE 2048


/*
 * send_all()
 *
 * Sends all bytes in the buffer.
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

        if (bytes_sent < 0 &&
            errno == EINTR)
        {
            continue;
        }

        return -1;
    }

    return 0;
}


/*
 * recv_line()
 *
 * Reads one newline-terminated response.
 */
int recv_line(int socket_fd,
              char *line,
              size_t line_size)
{
    size_t used = 0;

    while (used < line_size - 1)
    {
        char ch;

        ssize_t result =
            recv(socket_fd,
                 &ch,
                 1,
                 0);

        if (result > 0)
        {
            if (ch == '\n')
            {
                line[used] = '\0';

                if (used > 0 &&
                    line[used - 1] == '\r')
                {
                    line[used - 1] = '\0';
                }

                return 1;
            }

            line[used++] = ch;
            continue;
        }

        if (result == 0)
        {
            return 0;
        }

        if (errno == EINTR)
        {
            continue;
        }

        return -1;
    }

    return -1;
}


/*
 * receive_exact_file()
 *
 * Receives exactly file_size bytes and saves them.
 */
int receive_exact_file(int socket_fd,
                       const char *output_name,
                       long long file_size)
{
    FILE *file =
        fopen(output_name, "wb");

    if (file == NULL)
    {
        perror("fopen");
        return -1;
    }

    long long total_received = 0;

    while (total_received < file_size)
    {
        char buffer[BUFFER_SIZE];

        long long remaining =
            file_size - total_received;

        size_t wanted =
            sizeof(buffer);

        if (remaining < (long long)wanted)
        {
            wanted =
                (size_t)remaining;
        }

        ssize_t bytes_received =
            recv(socket_fd,
                 buffer,
                 wanted,
                 0);

        if (bytes_received > 0)
        {
            if (fwrite(buffer,
                       1,
                       (size_t)bytes_received,
                       file) !=
                (size_t)bytes_received)
            {
                fclose(file);
                return -1;
            }

            total_received +=
                bytes_received;

            continue;
        }

        if (bytes_received == 0)
        {
            fclose(file);
            return -1;
        }

        if (errno == EINTR)
        {
            continue;
        }

        fclose(file);
        return -1;
    }

    fclose(file);

    return 0;
}


int main(void)
{
    int controller_socket;

    struct sockaddr_in agent_address;

    char response[LINE_SIZE];


    controller_socket =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    if (controller_socket < 0)
    {
        perror("socket");
        return 1;
    }


    memset(&agent_address,
           0,
           sizeof(agent_address));

    agent_address.sin_family =
        AF_INET;

    agent_address.sin_port =
        htons(AGENT_PORT);


    if (inet_pton(AF_INET,
                  AGENT_IP,
                  &agent_address.sin_addr) <= 0)
    {
        perror("inet_pton");

        close(controller_socket);

        return 1;
    }


    if (connect(
            controller_socket,
            (struct sockaddr *)&agent_address,
            sizeof(agent_address)) < 0)
    {
        perror("connect");

        close(controller_socket);

        return 1;
    }


    printf("Connected to RemoteOps Agent at %s:%d\n",
           AGENT_IP,
           AGENT_PORT);


    /*
     * Send AUTH.
     */
    const char *auth_command =
        "AUTH OPS-1730\n";

    if (send_all(
            controller_socket,
            auth_command,
            strlen(auth_command)) < 0)
    {
        perror("send");

        close(controller_socket);

        return 1;
    }


    if (recv_line(
            controller_socket,
            response,
            sizeof(response)) <= 0)
    {
        printf("Failed to receive AUTH response\n");

        close(controller_socket);

        return 1;
    }


    printf("%s\n", response);


    /*
     * Request uploaded file.
     */
    const char *get_command =
        "GET upload_test.txt\n";

    if (send_all(
            controller_socket,
            get_command,
            strlen(get_command)) < 0)
    {
        perror("send");

        close(controller_socket);

        return 1;
    }


    if (recv_line(
            controller_socket,
            response,
            sizeof(response)) <= 0)
    {
        printf("Failed to receive GET response\n");

        close(controller_socket);

        return 1;
    }


    printf("%s\n", response);


    /*
     * Parse:
     *
     * OK FILE_SEND <filename> <filesize> SID:0371
     */
    char filename[256];
    long long file_size;
    char sid[64];

    if (sscanf(response,
               "OK FILE_SEND %255s %lld %63s",
               filename,
               &file_size,
               sid) != 3)
    {
        printf("Invalid GET response\n");

        close(controller_socket);

        return 1;
    }


    /*
     * Save the downloaded file with a new name.
     */
    const char *output_name =
        "downloaded_upload_test.txt";


    if (receive_exact_file(
            controller_socket,
            output_name,
            file_size) < 0)
    {
        printf("File download failed\n");

        close(controller_socket);

        return 1;
    }


    printf("Downloaded %s as %s (%lld bytes)\n",
           filename,
           output_name,
           file_size);


    close(controller_socket);

    return 0;
}

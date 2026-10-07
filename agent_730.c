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
 * Reads one newline-terminated line from TCP.
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

        /* Search existing buffered data for '\n' */
        for (i = 0; i < reader->used; i++)
        {
            if (reader->data[i] == '\n')
            {
                size_t line_length = i;

                if (line_length >= line_size)
                {
                    return -2;
                }

                memcpy(line,
                       reader->data,
                       line_length);

                line[line_length] = '\0';

                /* Remove optional '\r' */
                if (line_length > 0 &&
                    line[line_length - 1] == '\r')
                {
                    line[line_length - 1] = '\0';
                }

                /*
                 * Preserve bytes after this newline.
                 */
                memmove(reader->data,
                        reader->data + i + 1,
                        reader->used - (i + 1));

                reader->used -= (i + 1);

                return 1;
            }
        }

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

        if (bytes_received == 0)
        {
            if (reader->used == 0)
            {
                return 0;
            }

            return -3;
        }

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
 * Sends all bytes even if send()
 * sends only part of the buffer.
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
 * send_response()
 *
 * Sends one RemoteOps response line.
 * Adds SID:0371 and '\n' automatically.
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


/*
 * get_sysinfo()
 *
 * Reads CPU load, used memory and uptime
 * from Linux /proc files.
 */
int get_sysinfo(char *message,
                size_t message_size)
{
    FILE *file;

    double cpu_load;
    double uptime;

    long mem_total_kb = 0;
    long mem_available_kb = 0;
    long mem_used_mb;

    char label[64];
    long value;
    char unit[32];


    /*
     * Read 1-minute load average.
     */
    file = fopen("/proc/loadavg", "r");

    if (file == NULL)
    {
        return -1;
    }

    if (fscanf(file,
               "%lf",
               &cpu_load) != 1)
    {
        fclose(file);
        return -1;
    }

    fclose(file);


    /*
     * Read memory information.
     */
    file = fopen("/proc/meminfo", "r");

    if (file == NULL)
    {
        return -1;
    }

    while (fscanf(file,
                  "%63s %ld %31s",
                  label,
                  &value,
                  unit) == 3)
    {
        if (strcmp(label,
                   "MemTotal:") == 0)
        {
            mem_total_kb = value;
        }
        else if (strcmp(label,
                        "MemAvailable:") == 0)
        {
            mem_available_kb = value;
        }

        if (mem_total_kb > 0 &&
            mem_available_kb > 0)
        {
            break;
        }
    }

    fclose(file);

    if (mem_total_kb <= 0 ||
        mem_available_kb <= 0)
    {
        return -1;
    }

    /*
     * Used memory = total - available.
     * Convert KB to MB.
     */
    mem_used_mb =
        (mem_total_kb -
         mem_available_kb) / 1024;


    /*
     * Read uptime.
     */
    file = fopen("/proc/uptime", "r");

    if (file == NULL)
    {
        return -1;
    }

    if (fscanf(file,
               "%lf",
               &uptime) != 1)
    {
        fclose(file);
        return -1;
    }

    fclose(file);


    /*
     * Build SYSINFO message.
     *
     * send_response() will add SID:0371.
     */
    int length =
        snprintf(message,
                 message_size,
                 "OK SYSINFO %.2f %ld %.0f",
                 cpu_load,
                 mem_used_mb,
                 uptime);

    if (length < 0 ||
        (size_t)length >= message_size)
    {
        return -1;
    }

    return 0;
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

    int authenticated = 0;


    /*
     * Create TCP socket.
     */
    server_socket =
        socket(AF_INET,
               SOCK_STREAM,
               0);

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

    server_address.sin_family =
        AF_INET;

    server_address.sin_addr.s_addr =
        INADDR_ANY;

    server_address.sin_port =
        htons(AGENT_PORT);


    /*
     * Bind to port 9410.
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
     * Listen for Controller connections.
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
     * Accept one Controller for now.
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
     * Read and process commands.
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
             * Reject commands before AUTH.
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
             * SYSINFO command
             */
            if (strcmp(line,
                       "SYSINFO") == 0)
            {
                char sysinfo_message[LINE_SIZE];

                if (get_sysinfo(
                        sysinfo_message,
                        sizeof(sysinfo_message)) < 0)
                {
                    if (send_response(
                            client_socket,
                            "ERR 007 SYSINFO_FAILED") < 0)
                    {
                        perror("send");
                        break;
                    }
                }
                else
                {
                    if (send_response(
                            client_socket,
                            sysinfo_message) < 0)
                    {
                        perror("send");
                        break;
                    }
                }

                continue;
            }


            /*
             * Other authenticated commands
             * will be implemented later.
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

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
#define STORAGE_PATH "./agentfiles/IT24101730/"

/* Server settings */
#define BACKLOG 5
#define RECEIVE_BUFFER_SIZE 4096
#define LINE_SIZE 2048
#define RESPONSE_SIZE 8192
#define MAX_FILE_SIZE (10 * 1024 * 1024)

typedef struct
{
    char data[RECEIVE_BUFFER_SIZE];
    size_t used;
} LineReader;


/*
 * recv_line()
 *
 * Reads one newline-terminated line from TCP.
 */
int recv_line(int socket_fd,
              LineReader *reader,
              char *line,
              size_t line_size)
{
    while (1)
    {
        size_t i;

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

                if (line_length > 0 &&
                    line[line_length - 1] == '\r')
                {
                    line[line_length - 1] = '\0';
                }

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
 * Sends all bytes in a buffer.
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
 * Adds SID:0371 and newline automatically.
 */
int send_response(int socket_fd,
                  const char *message)
{
    char response[RESPONSE_SIZE];

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
 * Reads CPU load, used memory and uptime.
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

    file = fopen("/proc/loadavg", "r");

    if (file == NULL)
    {
        return -1;
    }

    if (fscanf(file, "%lf", &cpu_load) != 1)
    {
        fclose(file);
        return -1;
    }

    fclose(file);

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
        if (strcmp(label, "MemTotal:") == 0)
        {
            mem_total_kb = value;
        }
        else if (strcmp(label, "MemAvailable:") == 0)
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

    mem_used_mb =
        (mem_total_kb - mem_available_kb) / 1024;

    file = fopen("/proc/uptime", "r");

    if (file == NULL)
    {
        return -1;
    }

    if (fscanf(file, "%lf", &uptime) != 1)
    {
        fclose(file);
        return -1;
    }

    fclose(file);

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


/*
 * get_process_list()
 *
 * Gets a snapshot of running processes.
 */
int get_process_list(char *message,
                     size_t message_size)
{
    FILE *pipe;

    char process_line[256];
    char process_entry[256];

    int pid;
    char process_name[128];

    size_t used;

    int length =
        snprintf(message,
                 message_size,
                 "OK PROCS ");

    if (length < 0 ||
        (size_t)length >= message_size)
    {
        return -1;
    }

    used = (size_t)length;

    pipe = popen("ps -eo pid=,comm=", "r");

    if (pipe == NULL)
    {
        return -1;
    }

    while (fgets(process_line,
                 sizeof(process_line),
                 pipe) != NULL)
    {
        if (sscanf(process_line,
                   "%d %127s",
                   &pid,
                   process_name) != 2)
        {
            continue;
        }

        if (used > strlen("OK PROCS "))
        {
            if (used + 1 >= message_size)
            {
                break;
            }

            message[used] = ',';
            used++;
            message[used] = '\0';
        }

        length =
            snprintf(process_entry,
                     sizeof(process_entry),
                     "%d/%s",
                     pid,
                     process_name);

        if (length < 0)
        {
            pclose(pipe);
            return -1;
        }

        if (used + (size_t)length >= message_size)
        {
            break;
        }

        memcpy(message + used,
               process_entry,
               (size_t)length);

        used += (size_t)length;
        message[used] = '\0';
    }

    pclose(pipe);

    return 0;
}


/*
 * execute_whitelisted_command()
 *
 * Runs only allowed EXEC commands.
 */
int execute_whitelisted_command(const char *name,
                                char *message,
                                size_t message_size)
{
    const char *shell_command = NULL;

    FILE *pipe;

    char output[512];
    size_t used = 0;

    if (strcmp(name, "DATE") == 0)
    {
        shell_command = "date";
    }
    else if (strcmp(name, "UPTIME") == 0)
    {
        shell_command = "uptime";
    }
    else if (strcmp(name, "DISKFREE") == 0)
    {
        shell_command = "df -h /";
    }
    else if (strcmp(name, "HOSTNAME") == 0)
    {
        shell_command = "hostname";
    }
    else if (strcmp(name, "WHOAMI") == 0)
    {
        shell_command = "whoami";
    }
    else
    {
        return 1;
    }

    pipe = popen(shell_command, "r");

    if (pipe == NULL)
    {
        return -1;
    }

    output[0] = '\0';

    while (fgets(output + used,
                 sizeof(output) - used,
                 pipe) != NULL)
    {
        used = strlen(output);

        if (used >= sizeof(output) - 1)
        {
            break;
        }
    }

    pclose(pipe);

    for (size_t i = 0;
         output[i] != '\0';
         i++)
    {
        if (output[i] == '\n' ||
            output[i] == '\r')
        {
            output[i] = ' ';
        }
    }

    int length =
        snprintf(message,
                 message_size,
                 "OK EXEC_RESULT %s",
                 output);

    if (length < 0 ||
        (size_t)length >= message_size)
    {
        return -1;
    }

    return 0;
}


/*
 * receive_file_bytes()
 *
 * Receives exactly file_size raw bytes.
 */
int receive_file_bytes(int socket_fd,
                       LineReader *reader,
                       FILE *file,
                       long long file_size)
{
    long long total_received = 0;

    /*
     * First consume any bytes already stored
     * in the LineReader buffer.
     */
    if (reader->used > 0 &&
        file_size > 0)
    {
        size_t bytes_to_use =
            reader->used;

        if ((long long)bytes_to_use >
            file_size)
        {
            bytes_to_use =
                (size_t)file_size;
        }

        if (fwrite(reader->data,
                   1,
                   bytes_to_use,
                   file) != bytes_to_use)
        {
            return -1;
        }

        total_received +=
            (long long)bytes_to_use;

        memmove(reader->data,
                reader->data + bytes_to_use,
                reader->used - bytes_to_use);

        reader->used -=
            bytes_to_use;
    }

    /*
     * Receive remaining bytes.
     */
    while (total_received < file_size)
    {
        char buffer[4096];

        long long remaining =
            file_size - total_received;

        size_t wanted =
            sizeof(buffer);

        if (remaining <
            (long long)wanted)
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
                return -1;
            }

            total_received +=
                bytes_received;

            continue;
        }

        if (bytes_received == 0)
        {
            return -1;
        }

        if (errno == EINTR)
        {
            continue;
        }

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


    server_socket =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    if (server_socket < 0)
    {
        perror("socket");
        return 1;
    }


    memset(&server_address,
           0,
           sizeof(server_address));

    server_address.sin_family =
        AF_INET;

    server_address.sin_addr.s_addr =
        INADDR_ANY;

    server_address.sin_port =
        htons(AGENT_PORT);


    if (bind(server_socket,
             (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0)
    {
        perror("bind");
        close(server_socket);
        return 1;
    }


    if (listen(server_socket,
               BACKLOG) < 0)
    {
        perror("listen");
        close(server_socket);
        return 1;
    }


    printf("RemoteOps Agent listening on TCP port %d\n",
           AGENT_PORT);


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
             * AUTH
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
             * Require authentication.
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
             * SYSINFO
             */
            if (strcmp(line,
                       "SYSINFO") == 0)
            {
                char sysinfo_message[RESPONSE_SIZE];

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
             * LISTPROC
             */
            if (strcmp(line,
                       "LISTPROC") == 0)
            {
                char process_message[RESPONSE_SIZE];

                if (get_process_list(
                        process_message,
                        sizeof(process_message)) < 0)
                {
                    if (send_response(
                            client_socket,
                            "ERR 008 LISTPROC_FAILED") < 0)
                    {
                        perror("send");
                        break;
                    }
                }
                else
                {
                    if (send_response(
                            client_socket,
                            process_message) < 0)
                    {
                        perror("send");
                        break;
                    }
                }

                continue;
            }


            /*
             * EXEC
             */
            if (strncmp(line,
                        "EXEC ",
                        5) == 0)
            {
                const char *command_name =
                    line + 5;

                char exec_message[RESPONSE_SIZE];

                int exec_result =
                    execute_whitelisted_command(
                        command_name,
                        exec_message,
                        sizeof(exec_message));

                if (exec_result == 1)
                {
                    if (send_response(
                            client_socket,
                            "ERR 002 COMMAND_NOT_ALLOWED") < 0)
                    {
                        perror("send");
                        break;
                    }
                }
                else if (exec_result < 0)
                {
                    if (send_response(
                            client_socket,
                            "ERR 009 EXEC_FAILED") < 0)
                    {
                        perror("send");
                        break;
                    }
                }
                else
                {
                    if (send_response(
                            client_socket,
                            exec_message) < 0)
                    {
                        perror("send");
                        break;
                    }
                }

                continue;
            }


            /*
             * PUT
             */
            if (strncmp(line,
                        "PUT ",
                        4) == 0)
            {
                char filename[256];
                long long file_size;

                if (sscanf(line,
                           "PUT %255s %lld",
                           filename,
                           &file_size) != 2)
                {
                    if (send_response(
                            client_socket,
                            "ERR 010 INVALID_PUT") < 0)
                    {
                        perror("send");
                        break;
                    }

                    continue;
                }


                /*
                 * Prevent directory traversal.
                 */
                if (strchr(filename,
                           '/') != NULL ||
                    strstr(filename,
                           "..") != NULL)
                {
                    if (send_response(
                            client_socket,
                            "ERR 011 INVALID_FILENAME") < 0)
                    {
                        perror("send");
                        break;
                    }

                    continue;
                }


                /*
                 * Validate size.
                 */
                if (file_size < 0 ||
                    file_size > MAX_FILE_SIZE)
                {
                    if (send_response(
                            client_socket,
                            "ERR 004 FILE_TOO_LARGE") < 0)
                    {
                        perror("send");
                    }

                    break;
                }


                char file_path[512];

                int path_length =
                    snprintf(file_path,
                             sizeof(file_path),
                             "%s%s",
                             STORAGE_PATH,
                             filename);

                if (path_length < 0 ||
                    (size_t)path_length >=
                        sizeof(file_path))
                {
                    if (send_response(
                            client_socket,
                            "ERR 011 INVALID_FILENAME") < 0)
                    {
                        perror("send");
                        break;
                    }

                    continue;
                }


                FILE *upload_file =
                    fopen(file_path,
                          "wb");

                if (upload_file == NULL)
                {
                    if (send_response(
                            client_socket,
                            "ERR 012 FILE_WRITE_FAILED") < 0)
                    {
                        perror("send");
                        break;
                    }

                    continue;
                }


                if (receive_file_bytes(
                        client_socket,
                        &reader,
                        upload_file,
                        file_size) < 0)
                {
                    fclose(upload_file);

                    remove(file_path);

                    printf(
                        "File upload failed: %s\n",
                        filename);

                    break;
                }


                fclose(upload_file);


                char put_response[512];

                int response_length =
                    snprintf(
                        put_response,
                        sizeof(put_response),
                        "OK FILE_RECEIVED %s",
                        filename);

                if (response_length < 0 ||
                    (size_t)response_length >=
                        sizeof(put_response))
                {
                    break;
                }


                if (send_response(
                        client_socket,
                        put_response) < 0)
                {
                    perror("send");
                    break;
                }


                printf(
                    "File received: %s (%lld bytes)\n",
                    filename,
                    file_size);

                continue;
            }


            /*
             * Unknown authenticated command.
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


    close(client_socket);
    close(server_socket);

    return 0;
}

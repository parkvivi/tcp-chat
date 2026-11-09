
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define BUFFER_SIZE 1024

int sockfd;

int send_all(int fd, const char *data, size_t len) {
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, MSG_NOSIGNAL);

        if (n == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0)
            return -1;

        sent += (size_t)n;
    }

    return 0;
}

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    size_t pos = 0;

    (void)arg;

    while (1) {
        char c;
        ssize_t n = recv(sockfd, &c, 1, 0);

        if (n <= 0) {
            if (n == -1 && errno == EINTR)
                continue;
            break;
        }

        if (c == '\n') {
            buffer[pos] = '\0';
            pos = 0;
        } else if (c != '\r') {
            if (pos < sizeof(buffer) - 1)
                buffer[pos++] = c;
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    char name[32];
    char buffer[BUFFER_SIZE];
    pthread_t receiver;

    if (argc != 4) {
        fprintf(stderr, "Uso: %s <IP> <porta> <nome>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[2]);

    // nome do cliente é argumento de entrada
    snprintf(name, sizeof(name), "%s", argv[3]);
    double duration = 10.0;

    signal(SIGPIPE, SIG_IGN);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Endereco IPv4 invalido.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    // envio do nome do cliente
    if (send_all(sockfd, name, strlen(name)) == -1 || send_all(sockfd, "\n", 1) == -1) {
        perror("send");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (pthread_create(&receiver, NULL, receive_messages, NULL) != 0) {
        fprintf(stderr, "Erro ao criar thread receptora.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    // mensagem enviada continuamente pelo cliente
    snprintf(buffer, sizeof(buffer), "oiie de %s\n", name);
    size_t msg_len = strlen(buffer);

    struct timespec start_time, current_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    unsigned long long total_bytes_sent = 0;
    double elapsed_time = 0.0;

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        elapsed_time = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_nsec - start_time.tv_nsec) / 1e9;

        if (elapsed_time >= duration)
            break;

        if (send_all(sockfd, buffer, msg_len) == -1) {
            perror("send");
            break;
        }

        total_bytes_sent += msg_len;
    }

    shutdown(sockfd, SHUT_RDWR);
    pthread_join(receiver, NULL);
    close(sockfd);

    double mbps = (total_bytes_sent * 8.0) / (elapsed_time * 1e6);

    printf("[%s] Enviados: %llu bytes | Tempo: %.3fs | Taxa: %.3f Mbps\n", name, total_bytes_sent, elapsed_time, mbps);

    return EXIT_SUCCESS;
}

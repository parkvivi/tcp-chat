#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

typedef struct Client {
    int fd;
    int id;
    char name[32];
} Client;

int num_clients = 0;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

unsigned long long total_bytes_received_client[10] = {0};

// Variaveis para medir o throughput agregado da rodada
unsigned long long total_bytes_received = 0;
int clientes_na_rodada = 0;
int numero_rodada = 0;

struct timespec tempo_inicio, tempo_final;

/* Envia todos os bytes, mesmo se send() enviar parcialmente. */
int send_all(int fd, const char *data, size_t len) {
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent,
                         MSG_NOSIGNAL);

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

/* Le nome do cliente. */
ssize_t read_name(int fd, char *buffer, size_t capacity) {
    char nome[10] = {0};
    size_t received = 0;

    while (received < 9) {
        ssize_t n = recv(fd, nome + received, 9 - received, 0);
        if (n <= 0) {
            if (n == -1 && errno == EINTR)
                continue;
            return n;
        }
        received += (size_t)n;
    }

    nome[9] = '\0';
    snprintf(buffer, capacity, "%s", nome);

    return 9;
}

void *handle_client(void *arg) {
    Client *client = arg;
    char buffer[BUFFER_SIZE];

    /* A primeira linha recebida e o nome do usuario. */
    ssize_t n = read_name(client->fd, buffer, sizeof(buffer));

    if (n <= 0 || buffer[0] == '\0') {
        close(client->fd);
        free(client);
        return NULL;
    }

    snprintf(client->name, sizeof(client->name), "%s", buffer);

    pthread_mutex_lock(&clients_mutex);

    if (num_clients >= MAX_CLIENTS) {
        pthread_mutex_unlock(&clients_mutex);

        const char *error = "Servidor cheio.\n";
        send_all(client->fd, error, strlen(error));
        close(client->fd);
        free(client);
        return NULL;
    }

    sscanf(client->name, "Cliente_%d", &client->id);
    total_bytes_received_client[client->id] = 0;
    
    if (num_clients == 0) {
        total_bytes_received = 0;
        clientes_na_rodada = 0;
        numero_rodada++;

        clock_gettime(CLOCK_MONOTONIC, &tempo_inicio);
    }

    num_clients++;
    clientes_na_rodada++;

    pthread_mutex_unlock(&clients_mutex);

    printf("Usuario conectado: %s\n", client->name);

    while ((n = recv(client->fd, buffer, sizeof(buffer), 0)) > 0) { // recebendo mensagens!!
        total_bytes_received_client[client->id] += (size_t)n;
    }

    if (n == -1)
        perror("recv");

    printf("Usuario desconectado: %s | Total enviado: %llu bytes\n", client->name, total_bytes_received_client[client->id]);

    pthread_mutex_lock(&clients_mutex);
    num_clients--;
    total_bytes_received += total_bytes_received_client[client->id];

    if (num_clients == 0) { // ultimo cliente saiu!
        clock_gettime(CLOCK_MONOTONIC, &tempo_final);

        double tempo_total = (tempo_final.tv_sec - tempo_inicio.tv_sec) + (tempo_final.tv_nsec - tempo_inicio.tv_nsec) / 1e9;

        double throughput = (total_bytes_received * 8.0) / (tempo_total * 1e6);

        printf("\nRecebidos: %llu bytes | Tempo: %.3f | Throughput: %.3f\n", total_bytes_received, tempo_total, throughput);
        fflush(stdout);
    }

    pthread_mutex_unlock(&clients_mutex);

    close(client->fd);
    free(client);

    return NULL;
}

int main(int argc, char *argv[]) {
    int server_fd;
    struct sockaddr_in server_addr;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);

    signal(SIGPIPE, SIG_IGN);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Chat TCP ativo na porta %d.\n", port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(
            server_fd,
            (struct sockaddr *)&client_addr,
            &addr_len
        );

        if (client_fd == -1) {
            if (errno == EINTR)
                continue;

            perror("accept");
            continue;
        }

        Client *client = malloc(sizeof(Client));

        if (client == NULL) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        client->fd = client_fd;
        client->name[0] = '\0';

        pthread_t tid;

        if (pthread_create(&tid, NULL, handle_client, client) != 0) {
            fprintf(stderr, "Erro ao criar thread.\n");
            close(client_fd);
            free(client);
            continue;
        }

        pthread_detach(tid);
    }

    close(server_fd);
    pthread_mutex_destroy(&clients_mutex);

    return EXIT_SUCCESS;
}
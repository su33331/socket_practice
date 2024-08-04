#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#define PORT 8000
#define BACKLOG 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int connection_count = 0;

void *handle_client(void *arg) {
    int new_sockfd = *(int *)arg;
    free(arg);

    char buffer[256];
    int n;

    // クライアントに一意の番号を送信
    pthread_mutex_lock(&mutex);
    connection_count++;
    int connection_id = connection_count;
    pthread_mutex_unlock(&mutex);

    snprintf(buffer, sizeof(buffer), "%d", connection_id);
    n = send(new_sockfd, buffer, strlen(buffer), 0);
    if (n < 0) {
        perror("ERROR writing to socket");
        close(new_sockfd);
        return NULL;
    }

    close(new_sockfd);
    return NULL;
}

int main() {
    int sockfd, *new_sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    // ソケットを作成する
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    // ソケットにアドレスを割り当てる
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // クライアントからの接続を待つ
    if (listen(sockfd, BACKLOG) < 0) {
        perror("ERROR on listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        new_sockfd = (int *)malloc(sizeof(int));
        if (new_sockfd == NULL) {
            perror("ERROR allocating memory");
            continue;
        }

        clilen = sizeof(cli_addr);
        *new_sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (*new_sockfd < 0) {
            perror("ERROR on accept");
            free(new_sockfd);
            continue;
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, new_sockfd) != 0) {
            perror("ERROR creating thread");
            close(*new_sockfd);
            free(new_sockfd);
            continue;
        }

        pthread_detach(thread);
    }

    close(sockfd);
    return 0;
}

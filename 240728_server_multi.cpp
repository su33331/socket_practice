#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <random>
#include <vector>
#include <sstream>  // 追加

#define PORT 8000
#define BACKLOG 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int connection_count = 0;

// 数字の文字列を受け取り、ソートして再び文字列に変換する関数
std::string sortDigits(const std::string &input) {
    // 数字の文字列を整数のベクターに変換
    std::vector<int> digits;
    for (char ch : input) {
        if (std::isdigit(ch)) {
            digits.push_back(ch - '0'); // '0'を引いて整数に変換
        }
    }

    // 数字をソート
    std::sort(digits.begin(), digits.end());

    // ソートされた数字を文字列に変換
    std::ostringstream oss;
    for (int num : digits) {
        oss << num;
    }

    return oss.str();
}

void *handle_client(void *arg) {
    int new_sockfd = *(int *)arg;
    free(arg);

    char buffer[256];
    int n;

    // クライアントにコネクションIDを送信
    pthread_mutex_lock(&mutex);
    connection_count++;
    int connection_id = connection_count;
    pthread_mutex_unlock(&mutex);
    std::string content = "1:" + std::to_string(connection_id);

    snprintf(buffer, sizeof(buffer), "%s", content.c_str());
    n = send(new_sockfd, buffer, strlen(buffer), 0);
    if (n < 0) {
        perror("ERROR writing to socket");
        close(new_sockfd);
        return NULL;
    }

    // データの受信
    memset(buffer, 0, sizeof(buffer));
    n = recv(new_sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n < 0) {
        perror("ERROR reading from socket");
        close(new_sockfd);
        return NULL;
    }
    std::string received_data(buffer);
    std::string prefix = "2:";
    if (received_data.rfind(prefix, 0) == 0){
        std::string random_nums = received_data.substr(prefix.length());
        printf("Message from client: %s\n", random_nums.c_str());

        // ソートして再び送信
        std::string sorted_nums = sortDigits(random_nums);
        content = "3:" + sorted_nums;
        snprintf(buffer, sizeof(buffer), "%s", content.c_str());
        n = send(new_sockfd, buffer, strlen(buffer), 0);
        if (n < 0) {
            perror("ERROR writing to socket");
            close(new_sockfd);
            return NULL;
        }
    }
    close(new_sockfd);
    return NULL;
}

int main() {
    int sockfd;
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
        int *new_sockfd = (int *)malloc(sizeof(int));
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

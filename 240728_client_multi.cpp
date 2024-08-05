#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <random>
#include <sstream>  // std::ostringstream 用

#define PORT 8000
#define NUM_THREADS 5

std::string generateRandomList(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);

    std::string randomNumberString;
    for (int i = 0; i < length; ++i) {
        randomNumberString += std::to_string(dis(gen));
    }

    return randomNumberString;
}

void *client_thread(void *arg) {
    char *hostname = (char *)arg;
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    // ソケットを作成する
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return NULL;
    }

    // サーバーのホスト名を取得する
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        close(sockfd);
        return NULL;
    }

    // 接続先アドレスを設定する
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(PORT);

    // サーバーに接続する
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        close(sockfd);
        return NULL;
    }

    // データを受信する
    memset(buffer, 0, 256);
    n = recv(sockfd, buffer, 255, 0);
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        return NULL;
    }
    std::string received_data(buffer);

    // コネクションIDを取得
    std::string prefix1 = "1:";
    if (received_data.rfind(prefix1, 0) == 0) {
        std::string connection_id = received_data.substr(prefix1.length());
        printf("Message from server: %s\n", connection_id.c_str());

        // 文字列を整数に変換
        int receive_num = std::stoi(connection_id);

        // サーバーに数字列を送信
        std::string random_nums = generateRandomList(receive_num);
        std::string content = "2:" + random_nums;
        snprintf(buffer, sizeof(buffer), "%s", content.c_str());
        n = send(sockfd, buffer, strlen(buffer), 0);
        if (n < 0) {
            perror("ERROR writing to socket");
            close(sockfd);
            return NULL;
        }
    }

    // 次のデータを受信する
    memset(buffer, 0, 256);
    n = recv(sockfd, buffer, 255, 0);
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        return NULL;
    }
    received_data = buffer;

    // ソートされた数字列を処理
    std::string prefix2 = "3:";
    if (received_data.rfind(prefix2, 0) == 0) {
        std::string sorted_nums = received_data.substr(prefix2.length());
        printf("Message from server: %s\n", sorted_nums.c_str());
    }

    close(sockfd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage %s hostname\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        // スレッドごとに独自のホスト名を渡す
        if (pthread_create(&threads[i], NULL, client_thread, argv[1]) != 0) {
            perror("ERROR creating thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

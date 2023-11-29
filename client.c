#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 256

struct StockData {
    char date[20];
    double closing_price;
};

void read_stock_data(const char *file_name, struct StockData *stock_data, int *num_entries) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    char line[BUFFER_SIZE];
    int count = 0;
    fgets(line, BUFFER_SIZE, file);
    while (fgets(line, BUFFER_SIZE, file) != NULL) {
        char *token = strtok(line, ",");
        int token_count = 0;
        while (token != NULL) {
            if (token_count == 0) {
                strcpy(stock_data[count].date, token);
            } else if (token_count == 4) {
                stock_data[count].closing_price = atof(token);
            }
            token = strtok(NULL, ",");
            token_count++;
        }
        count++;
    }
    *num_entries = count;
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_address> <port>\n", argv[0]);
        return 1;
    }
    const char *serverAddress = argv[1];
    int port = atoi(argv[2]);
    struct StockData msft_stock[1000];
    struct StockData tsla_stock[1000];
    int msft_entries = 0, tsla_entries = 0;
    read_stock_data("MSFT.csv", msft_stock, &msft_entries);
    read_stock_data("TSLA.csv", tsla_stock, &tsla_entries);
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_address> <port>\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return 1;
    }
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, serverAddress, &server.sin_addr);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("Connection failed");
        close(sock);
        return 1;
    }
    char message[BUFFER_SIZE];
        while (1) {
        printf("> ");
        fflush(stdout);

        // read input
        fgets(message, BUFFER_SIZE, stdin);

        int is_empty = 1;
        for (int i = 0; message[i] != '\0'; ++i) {
            // check if the input is just spaces, newlines, or tabs
            if (message[i] != ' ' && message[i] != '\n' && message[i] != '\t') {
                is_empty = 0;
                break;
            }
        }
        if (is_empty) {
            // flush the input if the input is empty
            continue;
        }

        // send input to server
        send(sock, message, strlen(message), 0);

        // receive server response
        int bytes_received = recv(sock, message, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            break;
        }
        message[bytes_received] = '\0';
        printf("%s\n", message);
    }
    close(sock);
    return 0;
}

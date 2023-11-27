#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

/*
11/25 Update - Zahradinee
- Set up basic client server structure
- Need to do MaxProfits and Prices commands
- Currently has a bug where the prices command kills the terminal I'm not sure how to fix that

11/26 - Kevin
- Fixed the bugs related to command processing
  - It now handles invalid commands
  - The terminal doesn't kill after the second command anymore (it needed a while loop)
- The Prices command seems to work fine now!
- Fixed a little bit of the formatting to match the example output on the assignment spec
- Created the quit command, it quits both the client and server.

11/26 - Zahradinee
- Updated the code for the Max Profit command & created the calculate max profit function!
    - it currently works for the MSFT but not for the TSLA :(
*/

#define BUFFER_SIZE 256

struct StockData {
    char date[20];
    double closing_price;
};

int quit = 0;

void read_stock_data(const char *file_name, struct StockData *stock_data, int *num_entries) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char line[BUFFER_SIZE];
    int count = 0;

    // skip header line
    fgets(line, BUFFER_SIZE, file);

    // read csv 
    while (fgets(line, BUFFER_SIZE, file) != NULL) {
        char *token = strtok(line, ",");
        int token_count = 0;

        struct StockData stock_entry;

        while (token != NULL) {
            if (token_count == 0) {
                strcpy(stock_entry.date, token); // Assuming date is the first column
            } else if (token_count == 4) {
                stock_entry.closing_price = atof(token); // Assuming Close column is at index 4
            }
            token = strtok(NULL, ",");
            token_count++;
        }

        stock_data[count++] = stock_entry;
    }
    *num_entries = count;
    fclose(file);
}

double calculate_max_profit(struct StockData *stock_data, int num_entries, const char *start_date, const char *end_date) {
    double max_profit = -1.0;
    double min_price = -1.0;
    double max_price = -1.0;

    int buy_date_found = 0;
    int sell_date_found = 0;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(stock_data[i].date, start_date) == 0) {
            buy_date_found = 1;
            min_price = stock_data[i].closing_price;
        }

        if (strcmp(stock_data[i].date, end_date) == 0) {
            sell_date_found = 1;
            max_price = stock_data[i].closing_price;
        }
    }

    if (!buy_date_found || !sell_date_found || strcmp(start_date, end_date) >= 0) {
        return -1.0; // return -1.0 so no valid buying/selling date found/invalid date range
    }

    max_profit = max_price - min_price;
    return max_profit;
}

void handle_client_requests(int client_socket, struct StockData *msft_stock, int msft_entries, struct StockData *tsla_stock, int tsla_entries) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int msg_len = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (msg_len <= 0) {
            perror("Error reading from socket");
            close(client_socket);
            return;
        }
        buffer[msg_len] = '\0';

        char command[BUFFER_SIZE], stock[BUFFER_SIZE], date1[20];
        sscanf(buffer, "%s %s %s", command, stock, date1);

        if (strcmp(command, "quit") != 0) {
            printf("%s", buffer);
        }

        if (strcmp(command, "quit") == 0) {
            quit = 1;
            break;
        } else if (strcmp(command, "List") == 0) {
            char response[BUFFER_SIZE];
            snprintf(response, BUFFER_SIZE, "TSLA | MSFT");
            send(client_socket, response, strlen(response), 0);
        } else if (strcmp(command, "Prices") == 0) {
        if ((strcmp(stock, "MSFT") == 0 || strcmp(stock, "TSLA") == 0) && strlen(date1) == 10) {
            double price = -1.0;
            struct StockData *selected_stock = strcmp(stock, "MSFT") == 0 ? msft_stock : tsla_stock;
            int entries = strcmp(stock, "MSFT") == 0 ? msft_entries : tsla_entries;
            int found = 0;
            for (int i = 0; i < entries; ++i) {
                if (strcmp(selected_stock[i].date, date1) == 0) {
                    price = selected_stock[i].closing_price;
                    found = 1;
                    break;
                }
            }
            char response[BUFFER_SIZE];
            if (found) {
                // Adjust the price to have 2 digits after the decimal point
                snprintf(response, BUFFER_SIZE, "%.2f", price);
            } else {
                snprintf(response, BUFFER_SIZE, "Unknown");
            }
            send(client_socket, response, strlen(response), 0);
        } else {
            char invalid_msg[] = "Invalid syntax or date format\n";
            send(client_socket, invalid_msg, strlen(invalid_msg), 0);
        }
        } else if (strcmp(command, "MaxProfit") == 0) {
            char date2[20];
            sscanf(buffer, "%s %s %s %s", command, stock, date1, date2);
            if ((strcmp(stock, "MSFT") == 0 || strcmp(stock, "TSLA") == 0) && strlen(date1) == 10 && strlen(date2) == 10) {
            struct StockData *selected_stock = strcmp(stock, "MSFT") == 0 ? msft_stock : tsla_stock;
            int entries = strcmp(stock, "MSFT") == 0 ? msft_entries : tsla_entries;
  
            double profit = calculate_max_profit(selected_stock, entries, date1, date2);
  
            char response[BUFFER_SIZE];
            if (profit >= 0.0) {
                snprintf(response, BUFFER_SIZE, "%.2f", profit);
            } else {
                snprintf(response, BUFFER_SIZE, "Max profit not found for the date range");
            }
            send(client_socket, response, strlen(response), 0);
        } else {
            char invalid_msg[] = "Invalid syntax or date format\n";
            send(client_socket, invalid_msg, strlen(invalid_msg), 0);
        }
            } else {
                char invalid_msg[] = "Invalid syntax";
                send(client_socket, invalid_msg, strlen(invalid_msg), 0);
            }
        }
        close(client_socket);
    }


int main(int argc, char *argv[]) {
    // port num
    if (argc != 4) {
        fprintf(stderr, "Usage: %s MSFT.csv TSLA.csv <port>\n", argv[0]);
        return 1;
    }
    struct StockData msft_stock[1000];
    struct StockData tsla_stock[1000];
    int msft_entries = 0, tsla_entries = 0;
    read_stock_data(argv[1], msft_stock, &msft_entries);
    read_stock_data(argv[2], tsla_stock, &tsla_entries);
    // socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // bind socket to a port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[3]));

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        close(server_socket);
        return 1;
    }
    printf("server started\n");
    while (!quit) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("Accept failed");
            close(server_socket);
            return 1;
        }

        handle_client_requests(client_socket, msft_stock, msft_entries, tsla_stock, tsla_entries);
    }

    close(server_socket);

    return 0;
}

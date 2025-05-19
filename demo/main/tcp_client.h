/* components/tcp_client/tcp_client.h */
#pragma once
#include <stdint.h>
#include <stddef.h>
#define SERVER_IP "47.113.229.47"
#define SERVER_PORT 3007
#define UUID "92f561fc-345d-4503-901b-a883bbc8cf8d"

void tcp_client_receive_task(void *pvParameters);
void tcp_client_send_task(void *pvParameters);
void prepare_data_to_send(char *data_to_send, size_t buffer_size);
void handle_received_command(const char *json_str);

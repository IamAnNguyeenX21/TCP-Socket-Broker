#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

void *recv_thread_function(void *arg) {
    int sock = *(int *)arg;
    char buffer[1024] = {0};
    while(1) {
        int valread = read(sock, buffer, 1024 - 1);
        if ((valread > 0) && (strncmp(buffer, "Goodbye!\n", 9) != 0)) {
            printf("\nServer trả lời: %s\n", buffer);
            printf("\nClient > ");
            fflush(stdout); 
            

        } else if ((valread == 0) || (strncmp(buffer, "Goodbye!\n", 9) == 0)) {
            printf("\nServer đã đóng kết nối.\n");
            break;
        }
        memset(buffer, 0, sizeof(buffer));
    }            

    return NULL;
}


int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    int topic_list[100] = {0}; 
    int topic_count = 0;
    char input[1024];

    printf("--- KHỞI TẠO TCP CLIENT ---\n");

    // 1. Tạo socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Lỗi tạo socket");
        return -1;
    }

    // 2. Cấu hình địa chỉ Server (127.0.0.1 : 8080)
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // inet_pton giúp chuyển đổi IP dạng chuỗi sang nhị phân chuẩn mạng
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Địa chỉ IP không hợp lệ hoặc không được hỗ trợ\n");
        return -1;
    }

    // 3. Kết nối đến Server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Kết nối thất bại! Server đã bật chưa?");
        return -1;
    }

    printf("Đã kết nối thành công đến Server!\n");
    printf("Gõ tin nhắn để gửi (Gõ 'exit' để thoát).\n\n");
    // Tạo thread để nhận phản hồi từ Server
    pthread_t recv_thread;
    if(pthread_create(&recv_thread, NULL, recv_thread_function, (void*)&sock) != 0)
    {
        perror("Lỗi tạo thread nhận");
        return -1;
    }
    pthread_detach(recv_thread);

    // 4. Vòng lặp giao tiếp
    while(1) {
        printf("\nClient > ");
        
        // Đọc dữ liệu nhập từ bàn phím
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        // Xóa ký tự \n thừa khi nhấn Enter
        input[strcspn(input, "\n")] = '\0';

        // Gửi qua mạng
        // send(sock, input, strlen(input), 0);

        // Logic thoát
        if (strncmp(input, "exit", 4) == 0) {
            printf("Đang ngắt kết nối...\n");
            send(sock, input, strlen(input), 0);
            sleep(1);
            break;
        }

        // Xử lý đăng ký và hủy đăng ký topics
        if(strncmp(input, "0x00", 4) == 0)
        {
            int topic_id = atoi(input + 4);
            if(topic_id > 0 && topic_id < 100)
            {
                printf("Đăng ký topic %d\n", topic_id);
                topic_list[topic_count++] = topic_id;
                send(sock, input, strlen(input), 0);

            }
        }
        else if(strncmp(input, "0x01", 4) == 0)
        {
            int topic_id = atoi(input + 4);
            if(topic_id > 0 && topic_id < 100)
            {
                printf("Hủy đăng ký topic %d\n", topic_id);
                for(int i = 0; i < topic_count; i++)
                {
                    if(topic_list[i] == topic_id)
                    {
                        for(int j = i; j < topic_count -1; j++)
                        {
                            topic_list[j] = topic_list[j+1];
                        }
                        break;
                    }
                }
                topic_count--;
                send(sock, input, strlen(input), 0);
            }
        }
        else
        {
            // Ghép message theo định dạng topic_id+message và gửi
            char message[sizeof(input) + 1];
            for(int i = 0; i < topic_count; i++)
            {
                snprintf(message, sizeof(message), "%d%s", topic_list[i], input);
                send(sock, message, strlen(message), 0);
            }
        }
    }
    // 5. Đóng socket
    close(sock);
    return 0;
}
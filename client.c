#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
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

    // 4. Vòng lặp giao tiếp
    while(1) {
        printf("Client > ");
        
        // Đọc dữ liệu nhập từ bàn phím
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        // Xóa ký tự \n thừa khi nhấn Enter
        input[strcspn(input, "\n")] = '\0';

        // Gửi qua mạng
        send(sock, input, strlen(input), 0);

        // Logic thoát
        if (strncmp(input, "exit", 4) == 0) {
            printf("Đang ngắt kết nối...\n");
            break;
        }

        // Chờ nhận phản hồi từ Server
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, 1024 - 1);
        if (valread > 0) {
            printf("Server trả lời: %s", buffer);
        } else if (valread == 0) {
            printf("Server đã đóng kết nối.\n");
            break;
        }
    }

    // 5. Đóng socket
    close(sock);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>   // Thư viện epoll
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include "routing_table.h"

#define PORT 8080
#define MAX_EVENTS 100   // Số lượng sự kiện tối đa epoll_wait có thể lấy ra trong 1 lần

routing_information* routing_table[100];
volatile bool server_running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\n[System] Bắt đầu dọn dẹp Server...\n");
        server_running = 0;
    }
}

int main () {
    printf("--- Khởi động Server (Chế độ epoll tối thượng) ---\n");
    
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 
    sigaction(SIGINT, &sa, NULL);
    
    for(int i = 0; i < 100; i++) routing_table[i] = NULL;

    int server_fd, new_socket;
    struct sockaddr_in server_address;
    int opt = 1;
    socklen_t addrlen = sizeof(server_address);
    char buffer[1024];

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
    listen(server_fd, 10);

    // =========================================================
    // 1. KHỞI TẠO EPOLL
    // =========================================================
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }

    // Đăng ký server_fd vào epoll để theo dõi kết nối mới
    struct epoll_event ev;
    ev.events = EPOLLIN; // Chỉ quan tâm sự kiện đọc
    ev.data.fd = server_fd; // Lưu cái fd này vào event để sau này epoll_wait nhả ra cho mình biết
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }

    // Mảng để nhận danh sách các sự kiện từ Kernel trả về
    struct epoll_event events[MAX_EVENTS];

    printf("[System] Server đang lắng nghe trên cổng %d\n", PORT);

    // =========================================================
    // 2. VÒNG LẶP SỰ KIỆN (EVENT LOOP)
    // =========================================================
    while (server_running)
    {
        // Chờ sự kiện. Trả về đúng số lượng socket đang "phát sáng"
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (nfds == -1) {
            if (errno == EINTR) break; 
            perror("epoll_wait");
            break;
        }

        // 3. XỬ LÝ SỰ KIỆN: Chỉ lặp ĐÚNG bằng số lượng nfds (độ phức tạp O(1) so với tổng số socket)
        for (int n = 0; n < nfds; ++n) 
        {
            int current_fd = events[n].data.fd;

            // TRƯỜNG HỢP A: Có người kết nối mới
            if (current_fd == server_fd) 
            {
                new_socket = accept(server_fd, (struct sockaddr*)&server_address, &addrlen);
                if (new_socket == -1) {
                    perror("accept");
                    continue;
                }
                printf(">> [Kết nối] Client FD %d đã vào.\n", new_socket);
                
                // Đăng ký khách mới vào hệ thống epoll
                ev.events = EPOLLIN;
                ev.data.fd = new_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                    perror("epoll_ctl: add client");
                    close(new_socket);
                }
            } 
            // TRƯỜNG HỢP B: Có tin nhắn từ Client cũ
            else 
            {
                memset(buffer, 0, sizeof(buffer));
                ssize_t received_byte = read(current_fd, buffer, 1023);

                // Khách thoát hoặc lỗi
                if (received_byte <= 0 || strncmp(buffer, "exit", 4) == 0) 
                {
                    printf("<< [Ngắt kết nối] Client FD %d đã rời đi.\n", current_fd);
                    
                    // Gỡ bỏ khỏi epoll TRƯỚC khi close()
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                    close(current_fd);

                    // Xóa khỏi Bảng định tuyến
                    for(int j = 0; j < 100; j++) {
                        check_and_delete(&routing_table[j], current_fd);
                    }
                }
                // Xử lý Logic PUB/SUB như bình thường...
                else 
                {
                    buffer[received_byte] = '\0';
                    // (Toàn bộ logic if 0x00, 0x01, PUB đặt ở đây - giống hệt phiên bản trước)
                    // ...
                    send(current_fd, "OK!\n", 4, 0);
                }
            }
        }
    }

    // =========================================================
    // 4. DỌN DẸP
    // =========================================================
    close(server_fd);
    close(epoll_fd); // Nhớ đóng cả fd của bản thân epoll!
    // Xóa bộ nhớ routing_table...
    
    printf("[System] Server đã tắt an toàn.\n");
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define LISTEN_PORT 88
#define BUFFER_SIZE 64*1024

int server_socket;

//设备地址
const unsigned int name_len = 6;
const char *camera_name = "camera";
const char *player_name = "player";
struct sockaddr_in camera_address, player_address;

int player_num = 0;
int creat_udp_server(int port)
{
    struct sockaddr_in server_address;

    // 创建UDP套接字
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("Error creating socket");
        return -1;
    }

    // 设置服务器地址结构
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    // 绑定套接字到本地地址和端口
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        printf("Error binding socket");
        close(server_socket);
        return -1;
    }

    printf("UDP server is listening on port %d\n", port);
}

int verify_device_info(struct sockaddr_in *address, void *buf)
{
    size_t  buf_len = strlen((char *)buf);
    //判断消息是否为设备描述信息
    if(buf_len > 6)
        return -1;

    //是否为视频播放器
    if(strncmp(buf,player_name,name_len) == 0){
        memset(&player_address, 0, sizeof(player_address));
        player_address.sin_addr = address->sin_addr;
        player_address.sin_port = address->sin_port;
        player_num++;
        printf("player info : %s:%d\n", inet_ntoa(address->sin_addr), ntohs(address->sin_port));
        return 0;
    }
    //是否为摄像头
    if(strncmp(buf,camera_name,name_len) == 0){
        memset(&camera_address, 0, sizeof(camera_address));
        camera_address.sin_addr = address->sin_addr;
        camera_address.sin_port = address->sin_port;
        printf("camera info: %s:%d\n", inet_ntoa(address->sin_addr), ntohs(address->sin_port));
        return 0;
    }
}

int main() {
    if(creat_udp_server(LISTEN_PORT) == -1){
        printf("creat udp server fail\n");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_address;
    memset(&client_address, 0, sizeof(client_address));
    socklen_t client_address_len = sizeof(client_address);

    int recvlen = 0;
    while (1) {
        memset(buffer, 0, recvlen);
        // 接收数据
        recvlen = recvfrom(server_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_address, &client_address_len);
        if(recvlen == -1){
            perror("Error receiving data");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        verify_device_info(&client_address, buffer);
        // printf("Received message: %s\n", buffer);

        if(player_num == 0)
            continue;

        // 回传数据给客户端
        if(sendto(server_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&player_address, sizeof(player_address)) == -1){
            perror("Error sending data");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // printf("sent data back to %s:%d\n", inet_ntoa(player_address.sin_addr), ntohs(player_address.sin_port));
    }

    // 关闭套接字
    close(server_socket);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFSIZE 1024

void error_handling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(-1);
}

// 데이터를 보내는 스레드 함수 (키보드 입력 -> 서버 전송)
void* send_msg(void* arg)
{
    int sock = *(int*)arg;
    char msg[BUFSIZE];

    while(1)
    {
        fgets(msg, BUFSIZE, stdin);
        
        // 'q'나 'Q'를 누르면 종료되도록 예외 처리
        if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
        {
            close(sock);
            exit(0);
        }
        
        write(sock, msg, strlen(msg));
    }
    return NULL;
}

// 데이터를 받는 스레드 함수 (서버 수신 -> 화면 출력)
void* recv_msg(void* arg)
{
    int sock = *(int*)arg;
    char msg[BUFSIZE];
    int str_len;

    while(1)
    {
        str_len = read(sock, msg, BUFSIZE - 1);
        if(str_len <= 0) // 서버와 연결이 끊기거나 에러 발생 시
        {
            return (void*)-1;
        }
        msg[str_len] = '\0';
        fputs(msg, stdout);
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void* thread_return;

    if(argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(-1 == sock){
        error_handling("socket() error");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(-1 == connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
        error_handling("connect() error");
    }

    // 포인터로 안전하게 소켓 디스크립터를 넘기기 위해 동적 할당
    int* sock_param1 = (int*)malloc(sizeof(int));
    int* sock_param2 = (int*)malloc(sizeof(int));
    *sock_param1 = sock;
    *sock_param2 = sock;

    // 송신 스레드와 수신 스레드 생성
    pthread_create(&snd_thread, NULL, send_msg, sock_param1);
    pthread_create(&rcv_thread, NULL, recv_msg, sock_param2);

    // 스레드가 종료될 때까지 메인 함수가 기다려줌
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);

    close(sock);
    return 0;
}

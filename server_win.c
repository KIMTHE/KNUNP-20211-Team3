#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h> //inet_pton 
//#include <mutex>

// vs warning and winsock error 
#pragma comment(lib, "ws2_32.lib")
#pragma warning (disable : 4996)

//std::mutex push_lock;
//std::mutex erase_lock;
//std::mutex sock_lock;


int PORT_NUM = 50000;
#define BUF_SIZE 1024
#define CLIENT_SIZE 5
int READ = 3;
int WRITE = 5;

char* code_start = "********************\n*******<code>*******\n";
char* chat_start = "********************\n*******<chat>*******\n";
char* end = "********************\n";

char chat[20][100];
char code[20][100];

typedef struct    // socket info
{
    SOCKET hClntSock;
    SOCKADDR_IN clntAdr;
    CHAR name[20];
    CHAR ip[22];
} PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

typedef struct    // buffer info
{
    OVERLAPPED overlapped;
    WSABUF wsaBuf;
    char buffer[BUF_SIZE];
    INT rwMode;    // READ or WRITE
} PER_IO_DATA, * LPPER_IO_DATA;

// 유저리스트
int user_num = 0;
LPPER_HANDLE_DATA UserList[CLIENT_SIZE];

void ErrorHandling(const char* message);
unsigned __stdcall ThreadMain(void* CompletionPortIO);

int main(int argc, char* argv[])
{
    WSADATA    wsaData;
    HANDLE hComPort;
    SYSTEM_INFO sysInfo;
    LPPER_IO_DATA ioInfo;
    LPPER_HANDLE_DATA handleInfo;

    SOCKET hServSock;
    SOCKADDR_IN servAdr;
    DWORD recvBytes, flags = 0;
    INT i;

    // winsock start
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        ErrorHandling("WSAStartup Error");

    hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    GetSystemInfo(&sysInfo);

    // main thread와 연결된 thread 생성
    for (i = 0; i < sysInfo.dwNumberOfProcessors; i++)
        _beginthreadex(NULL, 0, ThreadMain, (LPVOID)hComPort, 0, NULL);

    // socket 설정
    hServSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAdr.sin_port = htons(PORT_NUM);

    // bind and listen q
    bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr));
    listen(hServSock, CLIENT_SIZE);

    while (1)
    {

        //sock_lock.lock();
        SOCKET hClntSock;
        SOCKADDR_IN clntAdr;
        int addrLen = sizeof(clntAdr);

        hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);

        //sock_lock.unlock();
        handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));        // LPPER_HANDLE_DATA 초기화
        inet_ntop(AF_INET, &clntAdr.sin_addr, handleInfo->ip, INET_ADDRSTRLEN);    // get new client ip
        handleInfo->hClntSock = hClntSock;                                // 클라이언트의 정보를 구조체에 담아 놓는다.
        memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

        // 소켓 입출력 포트에 accept을 통해서 return 된 클라이언트 정보를 묶는다.
        CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

        // 클라이언트가 가지게 될 data 초기화
        ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
        memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
        memset(ioInfo->buffer, 0x00, BUF_SIZE);
        ioInfo->wsaBuf.len = BUF_SIZE;
        ioInfo->wsaBuf.buf = ioInfo->buffer;
        ioInfo->rwMode = READ;

        // name 받기
        recv(handleInfo->hClntSock, handleInfo->name, 20, 0);

        // push_lock.lock();
        // 클라이언트 user data 초기화
        printf("새로운 유저가 입장했습니다 : %s, 현재 유저수 : %d\n", handleInfo->name, user_num + 1);
        UserList[user_num++] = handleInfo;
        //push_lock.unlock();

        // 비동기 입출력 시작
        WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
    }
    return 0;
}

unsigned __stdcall ThreadMain(void* pComPort)
{
    HANDLE hComPort = (HANDLE)pComPort;
    SOCKET sock;
    DWORD bytesTrans;
    LPPER_HANDLE_DATA handleInfo;
    LPPER_IO_DATA ioInfo;
    int flags = 0, i, j;
    CHAR message[BUF_SIZE];

    while (1)
    {
        // 비동기 입출력 완료를 기다린다.
        GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
        // 클라이언트의 socket을 가져온다.
        sock = handleInfo->hClntSock;

        // 첫 시작은 읽기 상태
        if (ioInfo->rwMode == READ)
        {
            //puts("\nmessage received!");
            if (bytesTrans == 0) //로그아웃시
            {
                //erase_lock.lock();
                for (i = 0; i < user_num;i++)
                {
                    if (UserList[i]->hClntSock == sock) 
                    {
                        printf("name : %s가 로그아웃함\n", UserList[i]->name);
                        for (j = i + 1; j <= user_num; j++) 
                        {
                            UserList[j - 1] = UserList[j];
                        }
                        user_num--;
                        break;
                    }
                }
                closesocket(sock);
                free(handleInfo);
                free(ioInfo);
                //erase_lock.unlock();
                continue;
            }
            memcpy(message, ioInfo->wsaBuf.buf, BUF_SIZE);
            message[bytesTrans] = '\0';            // 문자열의 끝에 \0을 추가한다 (쓰레기 버퍼 방지)

             //name 나누기
            //char *ptr = strtok(message, "]");    // [] => ']'기준으로 나눈다.
            //strcpy(handleInfo->name, ptr + 1);    // ]으로 나눈 name
            //ptr = strtok(NULL, "]");            // ]으로 다시 나눈 message
            //strcpy(message, ptr);                // message와 name이 나누어진다.
            
            printf("%s\n", message);
            //printf("Sock[%d] : %s\n", sock, message);

            // 클라이언트가 가진 데이터 구조체의 정보를 바꾼다.
            // 이젠 서버가 쓰기를 행함
            free(ioInfo);

            for (i=0; i<user_num; i++) //클라이언트에 메시지 뿌림
            {
                ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
                memset(&(ioInfo->overlapped), 0x00, sizeof(OVERLAPPED));
                
                ioInfo->rwMode = WRITE;

                if (bytesTrans == 0)
                {
                    closesocket(sock);
                    free(handleInfo);
                    free(ioInfo);
                    continue;
                }

                int len = strlen(message);
                ioInfo->wsaBuf.len = len;
                strcpy(ioInfo->buffer, message);
                ioInfo->wsaBuf.buf = ioInfo->buffer;

                if (WSASend(UserList[i]->hClntSock, &(ioInfo->wsaBuf), 1, &bytesTrans, 0, &(ioInfo->overlapped), NULL) == SOCKET_ERROR)
                {
                    if (WSAGetLastError() != WSA_IO_PENDING)
                        ErrorHandling("WSASend() error");
                }
            }

            // 데이터 구조체 초기화, 쓰기 -> 읽기 모드로 변경
            ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
            memset(&(ioInfo->overlapped), 0x00, sizeof(OVERLAPPED));
            ioInfo->wsaBuf.len = BUF_SIZE;
            ioInfo->wsaBuf.buf = ioInfo->buffer;
            ioInfo->rwMode = READ;

            // 읽기 시작
            if (WSARecv(sock, &(ioInfo->wsaBuf), 1, &bytesTrans, (LPDWORD)&flags, &(ioInfo->overlapped), NULL) == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                    ErrorHandling("WSASend() error");
            }
        }
        // 쓰기 상태
        else 
        {
            printf("Message Sent!\n");
            free(ioInfo);
        }
    }
    return 0;
}

void ErrorHandling(const char* message)
{
    perror(message);
    exit(1);
}

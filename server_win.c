#define _crt_secure_no_warnings

#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning (disable : 4996)

int PORT_NUM = 50000;
#define BUF_SIZE 1024
#define CLIENT_SIZE 5
int READ = 3;
int WRITE = 5;

char* code_start = "************************************************\n*********************<code>*********************\n\n";
char* chat_start = "************************************************\n*********************<chat>*********************\n\n";
char* end = "************************************************\n\n";

char chat[20][50] = { "","","" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" };
char code[20][50] = { "","","" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" ,"" };
char logmessage[10][50] = { "","","" ,"" ,"" ,"" ,"" ,"" ,"" ,"" };
int count = 0;
int l_count = 0;
char MES[5000];

FILE* source_file;
FILE* log_file;

typedef struct    // socket info
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
	CHAR name[20];
	CHAR ip[22];
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct    // buffer info
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	INT rwMode;    // READ or WRITE
} PER_IO_DATA, *LPPER_IO_DATA;

// 유저리스트
int user_num = 0;
LPPER_HANDLE_DATA UserList[CLIENT_SIZE];

void ErrorHandling(const char* message);
void MakeMessage();//클라이언트에게 보낼 내용(코드+채팅 합친것)을 생성
void MakeSource();//클라이언트가 소스코드 받기를 요청한 상태로 내용(코드)을 생성
void MakeLog();//클라이언트가 로그 받기를 요청한 상태로 내용(로그)을 생성
void Insertchat(char* M);//클라이언트에게 입력받은 채팅내용을 채팅 배열에 추가
void Insertlog(char* M);//클라이언트가 수정, 삭제를 요청한 경우 이러한 기록을 로그 배열에 추가
void modify_source();//서버의 데이터베이스(코드부분)을 수정
void modify_log();//서버의 데이터베이스(로그부분)을 수정
DWORD WINAPI ChatThreadMain(void* CompletionPortIO);

int main(int argc, char* argv[])
{
    system("mode con cols=80 lines=50"); //콘솔 크기 설정

    WSADATA    wsaData;
    HANDLE hComPort;
    SYSTEM_INFO sysInfo;
    LPPER_IO_DATA ioInfo;
    LPPER_HANDLE_DATA handleInfo;

    SOCKET hServSock;
    SOCKADDR_IN servAdr;
    DWORD recvBytes, flags = 0;
    INT i;

    //파일에서 소스코드 받아옴
    int exist;
    char* fname = "source file.txt";
	char* fname2 = "log file.txt";
    
    exist = access(fname, 0);

    if (exist == 0) //소스파일이 존재할때, 파일에서 코드를 받아옴
    {
        source_file = fopen(fname, "r");
        for (i = 0; i < 20; i++)
        {
            fgets(code[i], 50,source_file);

			if (code[i][strlen(code[i]) - 1] == '\n')
				code[i][strlen(code[i]) - 1] = '\0';
        }

		fclose(source_file);
    }

	exist = access(fname2, 0);

	if (exist == 0) //로그파일이 존재할때, 파일에서 코드를 받아옴
	{
		log_file = fopen(fname2, "r");

		fscanf(log_file, "%d", &l_count);
		fgetc(log_file);
		for (i = 0; i < l_count; i++)
		{
			fgets(logmessage[i], 50, log_file);
		}

		fclose(log_file);
	}

    // winsock start
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        ErrorHandling("WSAStartup Error");

    hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    GetSystemInfo(&sysInfo);

    // main thread와 연결된 thread 생성
    for (i = 0; i < sysInfo.dwNumberOfProcessors; i++)
        _beginthreadex(NULL, 0, ChatThreadMain, (LPVOID)hComPort, 0, NULL);

    // socket 설정
    hServSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAdr.sin_port = htons(PORT_NUM);

    // bind와 listen q
    bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr));
    listen(hServSock, CLIENT_SIZE);

    while (1)
    {
        SOCKET hClntSock;
        SOCKADDR_IN clntAdr;
        int addrLen = sizeof(clntAdr);

        hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);

        if (user_num >= 4)
        {
            closesocket(hClntSock);
            continue;
        }

        handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));        // LPPER_HANDLE_DATA 초기화
        inet_ntop(AF_INET, &clntAdr.sin_addr, handleInfo->ip, INET_ADDRSTRLEN);    // new client ip 가져옴
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

        // 클라이언트 user data 초기화
        char tmp[50];
        UserList[user_num++] = handleInfo;
        sprintf(tmp, "새로운 참가자분이 입장하셨습니다. %d/4\n", user_num);
        Insertchat(tmp);

        // 비동기 입출력 시작
        WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
    }
    return 0;

}

DWORD WINAPI ChatThreadMain(void* pComPort)
{
	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	int flags = 0, i, j, n;
	CHAR message[BUF_SIZE], T_message[BUF_SIZE], n_message[BUF_SIZE];

	while (1)
	{
		// 비동기 입출력 완료를 기다린다.
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		// 클라이언트의 socket을 가져온다.
		sock = handleInfo->hClntSock;

		// 첫 시작은 읽기 상태
		if (ioInfo->rwMode == READ)
		{
			if (bytesTrans == 0) //로그아웃시
			{
				for (i = 0; i < user_num; i++)
				{
					if (UserList[i]->hClntSock == sock)
					{
						for (j = i + 1; j <= user_num; j++)
						{
							UserList[j - 1] = UserList[j];
						}
						char tmp[50];
						user_num--;
						sprintf(tmp, "참가자 한분이 퇴장하셨습니다. %d/4\n", user_num);
						Insertchat(tmp);
						break;
					}
				}
				closesocket(sock);
				free(handleInfo);
				free(ioInfo);
				continue;
			}

			memcpy(message, ioInfo->wsaBuf.buf, BUF_SIZE);
			message[bytesTrans] = '\0';            // 문자열의 끝에 \0을 추가한다 (쓰레기 버퍼 방지)
			
			free(ioInfo);
			//진짜 메세지 부분 나누기
			strcpy(T_message, message);
			char *ptr = strtok(T_message, "]");    // [] => ']'기준으로 나눠서 닉네임부분 떼어냄
			ptr = strtok(NULL, " ");            // " "으로 다시 나눈 message
			strcpy(T_message, ptr);

			if (strcmp(T_message, "/modify") == 0) 
			{//메세지가 /modify로 시작하면 modify 명령어 수행
				ptr = strtok(NULL, " ");//modify가 몇번째 줄에서 일어나는지 적힌 부분 떼어냄
				strcpy(n_message, ptr);
				n = atoi(n_message);
				if (n < 21 && n>0) {//코드부분은 1-20줄 사이
					Insertlog(message);
					ptr = strtok(NULL, "\n");//수정 내용 떼어냄
					strcpy(code[n - 1], ptr);//코드 수정
					modify_source();//서버에 저장된 소스파일 수정
					modify_log();//서버에 저장된 로그파일 수정
				}
			}

			else if (strcmp(T_message, "/delete") == 0) //메세지가 /delete로 시작하면 delete 명령어 수행
			{
				ptr = strtok(NULL, " ");
				strcpy(n_message, ptr);
				n = atoi(n_message);
				if (n < 21 && n>0) {
					Insertlog(message);
					strcpy(code[n - 1], "");
					modify_source();
					modify_log();
				}
			}

			else if  (strcmp(T_message, "/get_log\n") == 0) //클라이언트가 /get_log를 입력
			{

				ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
				memset(&(ioInfo->overlapped), 0x00, sizeof(OVERLAPPED));
				ioInfo->rwMode = WRITE;
				MakeLog();//클라이언트에게 보낼 로그파일 생성
				int len = strlen(MES);
				ioInfo->wsaBuf.len = len;
				ioInfo->wsaBuf.buf = MES;

				if (WSASend(sock, &(ioInfo->wsaBuf), 1, &bytesTrans, 0, &(ioInfo->overlapped), NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
						ErrorHandling("WSASend() error");
				}
			}

			else if (strcmp(T_message, "/get_source\n") == 0)//클라이언트가 /get_source를 입력
			{

				ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
				memset(&(ioInfo->overlapped), 0x00, sizeof(OVERLAPPED));

				ioInfo->rwMode = WRITE;
				MakeSource();//클라이언트에게 보낼 소스파일 생성
				int len = strlen(MES);
				ioInfo->wsaBuf.len = len;
				ioInfo->wsaBuf.buf = MES;

				if (WSASend(sock, &(ioInfo->wsaBuf), 1, &bytesTrans, 0, &(ioInfo->overlapped), NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
						ErrorHandling("WSASend() error");
				}
			}
			else
				Insertchat(message);//클라이언트에게 받은 채팅 채팅배열에 추가

			MakeMessage();//클라이언트에게 보낼 메세지 생성

			// 클라이언트가 가진 데이터 구조체의 정보를 바꾼다.
			// 이젠 서버가 쓰기를 행함

			printf("%s", MES); //서버에 메시지 출력
			for (i = 0; i < user_num; i++) //클라이언트에 메시지 뿌림
			{
				ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
				memset(&(ioInfo->overlapped), 0x00, sizeof(OVERLAPPED));

				ioInfo->rwMode = WRITE;

				int len = strlen(MES);
				ioInfo->wsaBuf.len = len;
				ioInfo->wsaBuf.buf = MES;

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
			free(ioInfo);
		}
	}
	return 0;
}

void MakeMessage()//클라이언트에게 보낼 내용을 코드, 채팅 더해서 생성
{
	sprintf(MES, "%s1. %s\n2. %s\n3. %s\n4. %s\n5. %s\n6. %s\n7. %s\n8. %s\n9. %s\n10. %s\n11. %s\n12. %s\n13. %s\n14. %s\n15. %s\n16. %s\n17. %s\n18. %s\n19. %s\n20. %s\n%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		code_start, code[0], code[1], code[2], code[3], code[4], code[5], code[6], code[7], code[8], code[9], code[10], code[11], code[12], code[13], code[14], code[15], code[16],
		code[17], code[18], code[19], chat_start, chat[0], chat[1], chat[2], chat[3], chat[4], chat[5], chat[6], chat[7], chat[8], chat[9], chat[10], chat[11], chat[12], chat[13], chat[14], chat[15],
		chat[16], chat[17], chat[18], chat[19], end);
}

void MakeSource()// 코드내용 더해서 생성
{
	sprintf(MES, "/get_source\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s", code[0], code[1], code[2], code[3], code[4], code[5], code[6], code[7], code[8], code[9], code[10], code[11], code[12], code[13], code[14], code[15], code[16],
		code[17], code[18], code[19]);
}

void MakeLog()// 로그내용 더해서 생성
{
	sprintf(MES, "/get_log\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s", logmessage[0], logmessage[1], logmessage[2], logmessage[3], logmessage[4]
		, logmessage[5], logmessage[6], logmessage[7], logmessage[8], logmessage[9]);
}

void Insertchat(char* M)//채팅 배열에 M 추가
{
	int i;
	
	if (M[strlen(M) - 1] == '\n')
		M[strlen(M) - 1] == '\0';

	//처음 20개 다 찰때까지
	if (count < 20)
		strcpy(chat[count++], M);

	else
	{
		for (i = 0; i < 19; i++)
		{
			strcpy(chat[i], chat[i + 1]);//한칸씩 민다
		}
		strcpy(chat[19], M);
	}
}


void ErrorHandling(const char* message)
{
	perror(message);
	exit(1);
}

void Insertlog(char* M)//로그 배열에 M 추가
{

	int i;

	//처음 20개 다 찰때까지
	if (l_count < 10)
		strcpy(logmessage[l_count++], M);

	else
	{
		for (i = 0; i < 9; i++)
		{
			strcpy(logmessage[i], logmessage[i + 1]);//한칸씩 민다
		}
		strcpy(logmessage[9], M);
	}
}

void modify_source()//서버에 저장된 소스파일 업데이트
{
	char* fname = "source file.txt";
	int i;

	source_file = fopen(fname, "w");

	for (i = 0; i < 20; i++)
		fprintf(source_file, "%s\n", code[i]);

	fclose(source_file);
}

void modify_log()//서버에 저장된 로그 파일 업데이트
{
	char* fname = "log file.txt";
	int i;

	log_file = fopen(fname, "w");

	fprintf(log_file, "%d\n", l_count);

	for (i = 0; i < 10; i++)
		fprintf(log_file, "%s", logmessage[i]);

	fclose(log_file);
}
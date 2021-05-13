#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h> 

#define BUF_SIZE 100
#define NAME_SIZE 20

unsigned WINAPI SendMsg(void * arg);
unsigned WINAPI RecvMsg(void * arg);
void ErrorHandling(char * msg);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
	char server_ip[20];	//서버 ip 주소
	int port_num;		//서버 포트 번호
	char user_name[20];	//사용자 이름

	printf("사용할 이름을 입력해주세요 : ");
	scanf("%s", &user_name);
	printf("접속할 서버주소를 입력해주세요 : ");
	scanf(" %s", &server_ip);
	printf("접속할 포트번호를 입력해주세요 : ");
	scanf(" %s", &port_num);

	WSADATA wsaData;
	SOCKET hSock;
	SOCKADDR_IN servAdr;
	HANDLE hSndThread, hRcvThread;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	sprintf(name, "[%s]", user_name);
	hSock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr(server_ip);
	servAdr.sin_port = htons(atoi(port_num));

	if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");

	hSndThread =
		(HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSock, 0, NULL);
	hRcvThread =
		(HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSock, 0, NULL);

	WaitForSingleObject(hSndThread, INFINITE);
	WaitForSingleObject(hRcvThread, INFINITE);
	closesocket(hSock);
	WSACleanup();
	return 0;
}

unsigned WINAPI SendMsg(void * arg)   // send thread main
{
	SOCKET hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	while (1)
	{
		fgets(msg, BUF_SIZE, stdin);
		if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
		{
			closesocket(hSock);
			exit(0);
		}
		sprintf(nameMsg, "%s %s", name, msg);
		send(hSock, nameMsg, strlen(nameMsg), 0);
	}
	return 0;
}

unsigned WINAPI RecvMsg(void * arg)   // read thread main
{
	int count = 0;
	int i;
	int hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	int strLen;
	while (1)
	{
		strLen = recv(hSock, nameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		if (strLen == -1)
			return -1;

		if (strcmp(nameMsg, "********************") == 0) {
			count++;
		}	//칸 구분 문자열 검사

		if (count == 3) {
			system("cls");
		}	//3번 칸이 구분되면 화면 지우기

		nameMsg[strLen] = 0;
		fputs(nameMsg, stdout);
	}
	return 0;
}

void ErrorHandling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

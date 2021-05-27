#define _crt_secure_no_warnings

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h> 

#define BUF_SIZE 100
#define NAME_SIZE 20
int port_num = 50000;

unsigned WINAPI SendMsg(void * arg);//클라언트가 입력한 것을 서버에게 전송
unsigned WINAPI RecvMsg(void * arg);//서버가 전송한 내용을 받아서 출력, /get_code, /get_souce를 직전에 보냈었을 경우 해당 명령어도 같이 받아서 할 행동을 구분한다.
void ErrorHandling(char * msg);
void gotoxy(int x, int y);	//콘솔에서 채팅 입력 커서 위치 옮기는 함수

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];
FILE * log_file;
FILE* source_file;

int main(int argc, char *argv[])
{
	system("mode con cols=80 lines=50"); //콘솔 크기 설정
	char server_ip[20];	//서버 ip 주소
	char user_name[20];	//사용자 이름
	char nameMsg[NAME_SIZE + BUF_SIZE];

	printf("사용할 이름을 입력해주세요 : ");
	scanf("%s", &user_name);
	printf("접속할 서버주소를 입력해주세요 : ");
	scanf(" %s", &server_ip);

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
	servAdr.sin_port = htons(port_num);

	if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");

	//처음 메시지
	getchar();
	printf("\n\n*************************************\n************<명령어 목록>************\n");
	printf("\"/q\" or \"/Q\" : 종료\n");
	printf("\"modify [줄번호] [수정내용]\" : 내용수정\n");
	printf("\"delete [줄번호]\" : 내용삭제\n");
	printf("\"get_log\" : 수정로그파일 받기\n");
	printf("\"get_source\" : 소스파일 받기\n");
	printf("*************************************\n");
	printf("\n첫 메시지를 날려주세요 : ");
	fgets(msg, BUF_SIZE, stdin);
	sprintf(nameMsg, "%s %s", name, msg);
	send(hSock, nameMsg, strlen(nameMsg), 0);

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
		gotoxy(0, 50);
		fgets(msg, BUF_SIZE, stdin);
		if (!strcmp(msg, "/q\n") || !strcmp(msg, "/Q\n"))
		{
			closesocket(hSock);
			exit(0);
		}

		else if (strcmp(msg, "\n") == 0)
			continue;

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
	char nameMsg[5000], message[5000];
	int strLen;
	while (1)
	{
		gotoxy(0, 50); //커서위치 고정
		strLen = recv(hSock, nameMsg, 5000, 0);
		gotoxy(0, 50);
		if (strLen == -1)
			return -1;
		nameMsg[strLen] = 0;
		strcpy(message, nameMsg);
		char* ptr = strtok(message, "\n");    //\n 기준으로 앞부분 잘라냄

		if (strcmp(ptr, "/get_log") == 0)//클라이언트가 get_log를 요청한경우
		{
			ptr = strtok(NULL, "\n");
			log_file = fopen("log file.txt", "w");//로그파일 생성
			while (ptr != NULL)              //ptr이 NULL일때까지 (= strtok 함수가 NULL을 반환할때까지)
			{
				fprintf(log_file, "%s\n", ptr);         //자른 문자 출력
				ptr = strtok(NULL, "\n");     //자른 문자 다음부터 구분자 또 찾기
			}
			fclose(log_file);
		}
		else if (strcmp(ptr, "/get_source") == 0)//클라이언트가 get_source를 요청한경우
		{
			ptr = strtok(NULL, "\n");
			source_file = fopen("source file.txt", "w");//소스파일 생성
			while (ptr != NULL)              //ptr이 NULL일때까지 (= strtok 함수가 NULL을 반환할때까지)
			{
				fprintf(source_file, "%s\n", ptr);         //자른 문자 출력
				ptr = strtok(NULL, "\n");     //자른 문자 다음부터 구분자 또 찾기
			}
			fclose(source_file);
		}
		else//일반적인 경우
		{
			system("cls");//콘솔 화면 초기화
			gotoxy(0, 50);//커서 이동
			fputs(nameMsg, stdout);//출력
		}
	}
	return 0;
}

void ErrorHandling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void gotoxy(int x, int y) {
	COORD pos = { x,y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}
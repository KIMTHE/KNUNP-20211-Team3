#define _crt_secure_no_warnings

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h> 

#define BUF_SIZE 100
#define NAME_SIZE 20
int port_num = 50000;

unsigned WINAPI SendMsg(void * arg);//Ŭ���Ʈ�� �Է��� ���� �������� ����
unsigned WINAPI RecvMsg(void * arg);//������ ������ ������ �޾Ƽ� ���, /get_code, /get_souce�� ������ ���¾��� ��� �ش� ��ɾ ���� �޾Ƽ� �� �ൿ�� �����Ѵ�.
void ErrorHandling(char * msg);
void gotoxy(int x, int y);	//�ֿܼ��� ä�� �Է� Ŀ�� ��ġ �ű�� �Լ�

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];
FILE * log_file;
FILE* source_file;

int main(int argc, char *argv[])
{
	system("mode con cols=80 lines=50"); //�ܼ� ũ�� ����
	char server_ip[20];	//���� ip �ּ�
	char user_name[20];	//����� �̸�
	char nameMsg[NAME_SIZE + BUF_SIZE];

	printf("����� �̸��� �Է����ּ��� : ");
	scanf("%s", &user_name);
	printf("������ �����ּҸ� �Է����ּ��� : ");
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

	//ó�� �޽���
	getchar();
	printf("\n\n*************************************\n************<��ɾ� ���>************\n");
	printf("\"/q\" or \"/Q\" : ����\n");
	printf("\"modify [�ٹ�ȣ] [��������]\" : �������\n");
	printf("\"delete [�ٹ�ȣ]\" : �������\n");
	printf("\"get_log\" : �����α����� �ޱ�\n");
	printf("\"get_source\" : �ҽ����� �ޱ�\n");
	printf("*************************************\n");
	printf("\nù �޽����� �����ּ��� : ");
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
		gotoxy(0, 50); //Ŀ����ġ ����
		strLen = recv(hSock, nameMsg, 5000, 0);
		gotoxy(0, 50);
		if (strLen == -1)
			return -1;
		nameMsg[strLen] = 0;
		strcpy(message, nameMsg);
		char* ptr = strtok(message, "\n");    //\n �������� �պκ� �߶�

		if (strcmp(ptr, "/get_log") == 0)//Ŭ���̾�Ʈ�� get_log�� ��û�Ѱ��
		{
			ptr = strtok(NULL, "\n");
			log_file = fopen("log file.txt", "w");//�α����� ����
			while (ptr != NULL)              //ptr�� NULL�϶����� (= strtok �Լ��� NULL�� ��ȯ�Ҷ�����)
			{
				fprintf(log_file, "%s\n", ptr);         //�ڸ� ���� ���
				ptr = strtok(NULL, "\n");     //�ڸ� ���� �������� ������ �� ã��
			}
			fclose(log_file);
		}
		else if (strcmp(ptr, "/get_source") == 0)//Ŭ���̾�Ʈ�� get_source�� ��û�Ѱ��
		{
			ptr = strtok(NULL, "\n");
			source_file = fopen("source file.txt", "w");//�ҽ����� ����
			while (ptr != NULL)              //ptr�� NULL�϶����� (= strtok �Լ��� NULL�� ��ȯ�Ҷ�����)
			{
				fprintf(source_file, "%s\n", ptr);         //�ڸ� ���� ���
				ptr = strtok(NULL, "\n");     //�ڸ� ���� �������� ������ �� ã��
			}
			fclose(source_file);
		}
		else//�Ϲ����� ���
		{
			system("cls");//�ܼ� ȭ�� �ʱ�ȭ
			gotoxy(0, 50);//Ŀ�� �̵�
			fputs(nameMsg, stdout);//���
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
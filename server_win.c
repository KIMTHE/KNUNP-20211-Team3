#define _crt_secure_no_warnings

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

char* code_start = "************************\n*********<code>*********\n\n";
char* chat_start = "************************\n*********<chat>*********\n\n";
char* end = "************************\n\n";

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

// ��������Ʈ
int user_num = 0;
LPPER_HANDLE_DATA UserList[CLIENT_SIZE];

void ErrorHandling(const char* message);
void MakeMessage();
void MakeSource();
void MakeLog();
void Insertchat(char* M);
void Insertlog(char* M);
void modify_source();
void modify_log();
unsigned __stdcall ThreadMain(void* CompletionPortIO);

int main(int argc, char* argv[])
{
    system("mode con cols=80 lines=50"); //�ܼ� ũ�� ����

    WSADATA    wsaData;
    HANDLE hComPort;
    SYSTEM_INFO sysInfo;
    LPPER_IO_DATA ioInfo;
    LPPER_HANDLE_DATA handleInfo;

    SOCKET hServSock;
    SOCKADDR_IN servAdr;
    DWORD recvBytes, flags = 0;
    INT i;

    //���Ͽ��� �ҽ��ڵ� �޾ƿ�
    int exist;
    char* fname = "source file.txt";
	char* fname2 = "log file.txt";
    
    exist = access(fname, 0);

    if (exist == 0) //�ҽ������� �����Ҷ�, ���Ͽ��� �ڵ带 �޾ƿ�
    {
        source_file = fopen(fname, "r");

		fscanf(source_file, "%d", &count);

        for (i = 0; i < 20; i++)
        {
            fscanf(source_file,"%s", code[i]);
        }

		fclose(source_file);
    }

	exist = access(fname2, 0);

	if (exist == 0) //�α������� �����Ҷ�, ���Ͽ��� �ڵ带 �޾ƿ�
	{
		log_file = fopen(fname2, "r");

		fscanf(log_file, "%d", &l_count);

		for (i = 0; i < 10; i++)
		{
			fscanf(log_file, "%s", logmessage[i] );
		}

		fclose(log_file);
	}

    // winsock start
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        ErrorHandling("WSAStartup Error");

    hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    GetSystemInfo(&sysInfo);

    // main thread�� ����� thread ����
    for (i = 0; i < sysInfo.dwNumberOfProcessors; i++)
        _beginthreadex(NULL, 0, ThreadMain, (LPVOID)hComPort, 0, NULL);

    // socket ����
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

        if (user_num >= 4)
        {
            closesocket(hClntSock);
            continue;
        }

        //sock_lock.unlock();
        handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));        // LPPER_HANDLE_DATA �ʱ�ȭ
        inet_ntop(AF_INET, &clntAdr.sin_addr, handleInfo->ip, INET_ADDRSTRLEN);    // get new client ip
        handleInfo->hClntSock = hClntSock;                                // Ŭ���̾�Ʈ�� ������ ����ü�� ��� ���´�.
        memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

        // ���� ����� ��Ʈ�� accept�� ���ؼ� return �� Ŭ���̾�Ʈ ������ ���´�.
        CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

        // Ŭ���̾�Ʈ�� ������ �� data �ʱ�ȭ
        ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
        memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
        memset(ioInfo->buffer, 0x00, BUF_SIZE);
        ioInfo->wsaBuf.len = BUF_SIZE;
        ioInfo->wsaBuf.buf = ioInfo->buffer;
        ioInfo->rwMode = READ;

        // push_lock.lock();
        // Ŭ���̾�Ʈ user data �ʱ�ȭ
        char tmp[50];
        UserList[user_num++] = handleInfo;
        sprintf(tmp, "���ο� �����ں��� �����ϼ̽��ϴ�. %d/4\n", user_num);
        Insertchat(tmp);
        //push_lock.unlock();

        // �񵿱� ����� ����
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
	int flags = 0, i, j, n;
	CHAR message[BUF_SIZE], T_message[BUF_SIZE], n_message[BUF_SIZE];

	while (1)
	{
		// �񵿱� ����� �ϷḦ ��ٸ���.
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		// Ŭ���̾�Ʈ�� socket�� �����´�.
		sock = handleInfo->hClntSock;

		// ù ������ �б� ����
		if (ioInfo->rwMode == READ)
		{
			if (bytesTrans == 0) //�α׾ƿ���
			{
				//erase_lock.lock();
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
						sprintf(tmp, "������ �Ѻ��� �����ϼ̽��ϴ�. %d/4\n", user_num);
						Insertchat(tmp);
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
			message[bytesTrans] = '\0';            // ���ڿ��� ���� \0�� �߰��Ѵ� (������ ���� ����)
			Insertchat(message);

			//��¥ �޼��� �κ� ������
			strcpy(T_message, message);
			char *ptr = strtok(T_message, "]");    // [] => ']'�������� ������ �г��Ӻκ� ���
			ptr = strtok(NULL, " ");            // " "���� �ٽ� ���� message
			strcpy(T_message, ptr);

			if (strcmp(T_message, "/modify") == 0) 
			{//�޼����� /modify�� �����ϸ� modify ��ɾ� ����
				Insertlog(message);
				ptr = strtok(NULL, " ");//modify�� ���° �ٿ��� �Ͼ���� ���� �κ� ���
				strcpy(n_message, ptr);
				n = atoi(n_message);
				if (n < 21 && n>0) {//�ڵ�κ��� 1-20�� ����
					ptr = strtok(NULL, "\n");//���� ���� ���
					strcpy(code[n - 1], ptr);//�ڵ� ����
				}

				modify_source();
				modify_log();
			}

			else if (strcmp(T_message, "/delete") == 0) 
			{
				Insertlog(message);
				ptr = strtok(NULL, " ");
				strcpy(n_message, ptr);
				n = atoi(n_message);
				if (n < 21 && n>0) {
					strcpy(code[n - 1], "");
				}

				modify_source();
				modify_log();
			}

			else if  (strcmp(T_message, "/get_log") == 0) 
			{
				free(ioInfo);

				ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
				memset(&(ioInfo->overlapped), 0x00, sizeof(OVERLAPPED));

				ioInfo->rwMode = WRITE;

				int len = strlen(MES);
				ioInfo->wsaBuf.len = len;
				ioInfo->wsaBuf.buf = MES;

				if (WSASend(sock, &(ioInfo->wsaBuf), 1, &bytesTrans, 0, &(ioInfo->overlapped), NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
						ErrorHandling("WSASend() error");
				}
			}

			else if (strcmp(T_message, "/get_source") == 0)
			{
				free(ioInfo);

				ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
				memset(&(ioInfo->overlapped), 0x00, sizeof(OVERLAPPED));

				ioInfo->rwMode = WRITE;

				int len = strlen(MES);
				ioInfo->wsaBuf.len = len;
				ioInfo->wsaBuf.buf = MES;

				if (WSASend(sock, &(ioInfo->wsaBuf), 1, &bytesTrans, 0, &(ioInfo->overlapped), NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
						ErrorHandling("WSASend() error");
				}
			}

			MakeMessage();

			// Ŭ���̾�Ʈ�� ���� ������ ����ü�� ������ �ٲ۴�.
			// ���� ������ ���⸦ ����
			free(ioInfo);

			printf("%s", MES); //������ �޽��� ���
			for (i = 0; i < user_num; i++) //Ŭ���̾�Ʈ�� �޽��� �Ѹ�
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

			// ������ ����ü �ʱ�ȭ, ���� -> �б� ���� ����
			ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
			memset(&(ioInfo->overlapped), 0x00, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rwMode = READ;

			// �б� ����
			if (WSARecv(sock, &(ioInfo->wsaBuf), 1, &bytesTrans, (LPDWORD)&flags, &(ioInfo->overlapped), NULL) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
					ErrorHandling("WSASend() error");
			}
		}
		// ���� ����
		else
		{
			//printf("Message Sent!\n");
			free(ioInfo);
		}
	}
	return 0;
}

void MakeMessage()
{

	sprintf(MES, "%s1. %s\n2. %s\n3. %s\n4. %s\n5. %s\n6. %s\n7. %s\n8. %s\n9. %s\n10. %s\n11. %s\n12. %s\n13. %s\n14. %s\n15. %s\n16. %s\n17. %s\n18. %s\n19. %s\n20. %s\n%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		code_start, code[0], code[1], code[2], code[3], code[4], code[5], code[6], code[7], code[8], code[9], code[10], code[11], code[12], code[13], code[14], code[15], code[16],
		code[17], code[18], code[19], chat_start, chat[0], chat[1], chat[2], chat[3], chat[4], chat[5], chat[6], chat[7], chat[8], chat[9], chat[10], chat[11], chat[12], chat[13], chat[14], chat[15],
		chat[16], chat[17], chat[18], chat[19], end);
}

void MakeSource()
{
	sprintf(MES, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s", code[0], code[1], code[2], code[3], code[4], code[5], code[6], code[7], code[8], code[9], code[10], code[11], code[12], code[13], code[14], code[15], code[16],
		code[17], code[18], code[19]);
}

void MakeLog()
{
	sprintf(MES, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s", logmessage[0], logmessage[1], logmessage[2], logmessage[3], logmessage[4]
		, logmessage[5], logmessage[6], logmessage[7], logmessage[8], logmessage[9]);
}

void Insertchat(char* M)
{

	int i;

	//ó�� 20�� �� ��������
	if (count < 20)
		strcpy(chat[count++], M);

	else
	{
		for (i = 0; i < 19; i++)
		{
			strcpy(chat[i], chat[i + 1]);//��ĭ�� �δ�
		}
		strcpy(chat[19], M);
	}
}


void ErrorHandling(const char* message)
{
	perror(message);
	exit(1);
}

void Insertlog(char* M)
{

	int i;

	//ó�� 20�� �� ��������
	if (l_count < 10)
		strcpy(logmessage[l_count++], M);

	else
	{
		for (i = 0; i < 9; i++)
		{
			strcpy(logmessage[i], logmessage[i + 1]);//��ĭ�� �δ�
		}
		strcpy(logmessage[9], M);
	}
}

void modify_source()
{
	char* fname = "source file.txt";
	int i;

	source_file = fopen(fname, "w");

	fprintf(source_file, "%d\n", count);

	for (i = 0; i < 20; i++)
		fprintf(source_file, "%s\n", code[i]);

	fclose(source_file);
}

void modify_log()
{
	char* fname = "log file.txt";
	int i;

	log_file = fopen(fname, "w");

	fprintf(log_file, "%d\n", l_count);

	for (i = 0; i < 10; i++)
		fprintf(log_file, "%s\n", logmessage[i]);

	fclose(log_file);
}
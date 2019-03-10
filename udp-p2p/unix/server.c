/*
某局域网内客户端C1先与外网服务器S通信，S记录C1经NAT设备转换后在外网中的ip和port；
然后另一局域网内客户端C2与S通信，S记录C2经NAT设备转换后在外网的ip和port；S将C1的
外网ip和port给C2，C2向其发送数据包；S将C2的外网ip和port给C1，C1向其发送数据包，打
洞完成，两者可以通信。(C1、C2不分先后)
测试假设C1、C2已确定是要与对方通信，实际情况下应该通过C1给S的信息和C2给S的信息，S
判断是否给两者搭桥。（因为C1可能要与C3通信，此时需要等待C3的连接，而不是给C1和
C2搭桥）
编译：gcc UDPServer.c -o UDPServer -lws2_32
*/
//#include <Winsock2.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 5050
#define BUFFER_SIZE 100
 
int main() {
	//server即外网服务器
	int serverPort = DEFAULT_PORT;
	//WSADATA wsaData;	//??暂时不用
	int serverListen;
	struct sockaddr_in serverAddr;
 
	//检查协议栈
	/*
	if (WSAStartup(MAKEWORD(2,2),&wsaData) != 0 ) {
		printf("Failed to load Winsock.\n");
		return -1;
	}
	*/
	
	//建立监听socket
	serverListen = socket(AF_INET,SOCK_DGRAM,0);
	if (serverListen == -1) {
		printf("socket() failed\n");
		return -1;
	}
 
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
 
	if (bind(serverListen,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) == -1) {
		printf("bind() failed\n");
		return -1;
	}
 
	//接收来自客户端的连接，source1即先连接到S的客户端C1
	struct sockaddr_in sourceAddr1;
	int sourceAddrLen1 = sizeof(sourceAddr1);
	int sockC1 = socket(AF_INET,SOCK_DGRAM,0);
	char bufRecv1[BUFFER_SIZE];
	int len;
 
	len = recvfrom(serverListen, bufRecv1, sizeof(bufRecv1), 0,(struct sockaddr*)&sourceAddr1,&sourceAddrLen1);
	if (len < 0 ) {
		printf("recv() failed\n");
		return -1;
	}
 
	printf("C1 IP:[%s],PORT:[%d]\n",inet_ntoa(sourceAddr1.sin_addr), ntohs(sourceAddr1.sin_port));
 
	//接收来自客户端的连接，source2即后连接到S的客户端C2
	struct sockaddr_in sourceAddr2;
	int sourceAddrLen2 = sizeof(sourceAddr2);
	int sockC2 = socket(AF_INET,SOCK_DGRAM,0);
	char bufRecv2[BUFFER_SIZE];
 
	len = recvfrom(serverListen, bufRecv2, sizeof(bufRecv2), 0,(struct sockaddr*)&sourceAddr2,&sourceAddrLen2);
	if (len < 0) {
		printf("recv() failed\n");
		return -1;
	}
 
	printf("C2 IP:[%s],PORT:[%d]\n",inet_ntoa(sourceAddr2.sin_addr), ntohs(sourceAddr2.sin_port));	
	
	//向C1发送C2的外网ip和port
	char bufSend1[BUFFER_SIZE];//bufSend1中存储C2的外网ip和port
	memset(bufSend1,'\0',sizeof(bufSend1));
	char* ip2 = inet_ntoa(sourceAddr2.sin_addr);//C2的ip
	char port2[10];//C2的port
	
	//itoa(ntohs(sourceAddr2.sin_port),port2,10);//10代表10进制
	sprintf(port2, "%d", ntohs(sourceAddr2.sin_port));
	for (int i=0;i<strlen(ip2);i++) {
		bufSend1[i] = ip2[i];
	}
	bufSend1[strlen(ip2)] = '^';
	for (int i=0;i<strlen(port2);i++) {
		bufSend1[strlen(ip2) + 1 + i] = port2[i];
	}

	printf("=>:%s", bufSend1);
	len = sendto(sockC1,bufSend1,sizeof(bufSend1),0,(struct sockaddr*)&sourceAddr1,sizeof(sourceAddr1));
	if (len < 0) {
		printf("send() failed\n");
		return -1;
	} else if (len == 0) {
		return -1;
	} else {
		printf("send() byte:%d\n",len);
	}
 
	//向C2发送C1的外网ip和port
	char bufSend2[BUFFER_SIZE];//bufSend2中存储C1的外网ip和port
	memset(bufSend2,'\0',sizeof(bufSend2));
	char* ip1 = inet_ntoa(sourceAddr1.sin_addr);//C1的ip
	char port1[10];//C1的port
	//itoa(ntohs(sourceAddr1.sin_port),port1,10);
	sprintf(port1, "%d", ntohs(sourceAddr1.sin_port));
	for (int i=0;i<strlen(ip1);i++) {
		bufSend2[i] = ip1[i];
	}
	bufSend2[strlen(ip1)] = '^';
	for (int i=0;i<strlen(port1);i++) {
		bufSend2[strlen(ip1) + 1 + i] = port1[i];
	}
	
	printf("=>:%s", bufSend2);
	len = sendto(sockC2,bufSend2,sizeof(bufSend2),0,(struct sockaddr*)&sourceAddr2,sizeof(sourceAddr2));
	if (len < 0) {
		printf("send() failed\n");
		return -1;
	} else if (len == 0) {
		return -1;
	} else {
		printf("send() byte:%d\n",len);
	}
 
	//server的中间人工作已完成，退出即可，剩下的交给C1与C2相互通信
	//closesocket(serverListen);
	//closesocket(sockC1);
	//closesocket(sockC2);
	//WSACleanup();
 	close(serverListen);
	close(sockC1);
	close(sockC2);

	return 0;
}

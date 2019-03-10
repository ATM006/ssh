/*
客户端C1，连接到外网服务器S，并从S的返回信息中得到它想要连接的C2的外网ip和port，然后
C1给C2发送数据包进行连接。
*/
#include<Winsock2.h>
#include<stdio.h>
#include<stdlib.h>
 
#define PORT 7777
#define BUFFER_SIZE 100
 
//调用方式:UDPClient1 10.2.2.2 5050 (外网服务器S的ip和port)
int main(int argc,char* argv[]) {
	WSADATA wsaData;
	struct sockaddr_in serverAddr;
	struct sockaddr_in thisAddr;
 
	thisAddr.sin_family = AF_INET;
	thisAddr.sin_port = htons(PORT);
	thisAddr.sin_addr.s_addr = htonl(INADDR_ANY);
 
	if (argc<3) {
		printf("Usage: client1[server IP address , server Port]\n");
		return -1;
	}
 
	if (WSAStartup(MAKEWORD(2,2),&wsaData) != 0) {
		printf("Failed to load Winsock.\n");
		return -1;
	}
 
	//初始化服务器S信息
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
	
	//建立与服务器通信的socket和与客户端通信的socket
	SOCKET sockS = socket(AF_INET,SOCK_DGRAM,0);
	if (sockS == INVALID_SOCKET) {
		printf("socket() failed:%d\n",WSAGetLastError());
		return -1;
	}
	if (bind(sockS,(LPSOCKADDR)&thisAddr,sizeof(thisAddr)) == SOCKET_ERROR) {
		printf("bind() failed:%d\n",WSAGetLastError());
		return -1;
	}
	SOCKET sockC = socket(AF_INET,SOCK_DGRAM,0);
	if (sockC == INVALID_SOCKET) {
		printf("socket() failed:%d\n",WSAGetLastError());
		return -1;
	}
 
	char bufSend[] = "I am C1";
	char bufRecv[BUFFER_SIZE];
	memset(bufRecv,'\0',sizeof(bufRecv));
	struct sockaddr_in sourceAddr;//暂存接受数据包的来源，在recvfrom中使用
	int sourceAddrLen = sizeof(sourceAddr);//在recvfrom中使用
	struct sockaddr_in oppositeSideAddr;//C2的地址信息
 
	int len;
	
	//C1给S发送数据包
	len = sendto(sockS,bufSend,sizeof(bufSend),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
	if (len == SOCKET_ERROR) {
		printf("sendto() failed:%d\n", WSAGetLastError());
		return -1;
	}
 
	//C1从S返回的数据包中得到C2的外网ip和port
	len = recvfrom(sockS, bufRecv, sizeof(bufRecv), 0,(struct sockaddr*)&sourceAddr,&sourceAddrLen);
	if (len == SOCKET_ERROR) {
		printf("recvfrom() failed:%d\n", WSAGetLastError());
		return -1;
	}
 
	//下面的处理是由于测试环境（本机+两台NAT联网的虚拟机）原因，若在真实环境中不需要这段处理。
	/*
	  关闭与服务器通信的socket，并把与C2通信的socket绑定到相同的端口，真实环境中，路由器的NAT会将客户端
	  对外的访问从路由器的外网ip某固定端口发送出去，并在此端口接收
	*/
	closesocket(sockS);
	if (bind(sockC,(LPSOCKADDR)&thisAddr,sizeof(thisAddr)) == SOCKET_ERROR) {
		printf("bind() failed:%d\n",WSAGetLastError());
		return -1;
	}
 
	char ip[20];
	char port[10];
	int i;
	for (i=0;i<strlen(bufRecv);i++)
		if (bufRecv[i] != '^')
			ip[i] = bufRecv[i];
		else break;
	ip[i] = '\0';
	int j;
	for (j=i+1;j<strlen(bufRecv);j++)
		port[j - i - 1] = bufRecv[j];
	port[j - i - 1] = '\0';
 
	oppositeSideAddr.sin_family = AF_INET;
	oppositeSideAddr.sin_port = htons(atoi(port));
	oppositeSideAddr.sin_addr.s_addr = inet_addr(ip);
 
	//下面的处理是由于测试环境（本机+两台NAT联网的虚拟机）原因，若在真实环境中不需要这段处理。
	/*
	  此处由于是在本机，ip为127.0.0.1，但是如果虚拟机连接此ip的话，是与虚拟机本机通信，而不是
	  真实的本机，真实本机即此实验中充当NAT的设备，ip为10.0.2.2。
	*/
	oppositeSideAddr.sin_addr.s_addr = inet_addr("10.0.2.2");
 
	//设置sockC为非阻塞
	unsigned long ul = 1;
	ioctlsocket(sockC, FIONBIO, (unsigned long*)&ul);
 
	//C1向C2不停地发出数据包，得到C2的回应，与C2建立连接
	while (1) {
		Sleep(1000);
		//C1向C2发送数据包
		len = sendto(sockC,bufSend,sizeof(bufSend),0,(struct sockaddr*)&oppositeSideAddr,sizeof(oppositeSideAddr));
		if (len == SOCKET_ERROR) {
			printf("while sending package to C2 , sendto() failed:%d\n", WSAGetLastError());
			return -1;
		}else {
			printf("successfully send package to C2\n");
		}
	
		//C1接收C2返回的数据包，说明C2到C1打洞成功，C2可以直接与C1通信了
		len = recvfrom(sockC, bufRecv, sizeof(bufRecv), 0,(struct sockaddr*)&sourceAddr,&sourceAddrLen);
		if (len == WSAEWOULDBLOCK) {
			continue;//未收到回应
		}else {
			printf("C2 IP:[%s],PORT:[%d]\n",inet_ntoa(sourceAddr.sin_addr)
				,ntohs(sourceAddr.sin_port));
			printf("C2 says:%s\n",bufRecv);
			
		}
	}
 
	closesocket(sockC);
 
	
}

#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include"pub.h"

#define ERR_EXIT(m) \
		do \
		{    \
			perror(m); \
			exit(EXIT_FAILURE); \
		}while(0)

//�û��б�
USER_LIST client_list;
			
void do_login(MESSAGE & msg,int sock,struct sockaddr_in * cliaddr);
void do_logout(MESSAGE & msg,int sock,struct sockaddr_in * cliaddr);
void do_sendlist(int sock,struct sockaddr_in * cliaddr);
			
void chat_srv(int sock)
{
	struct sockaddr_in cliaddr;
	socklen_t clilen=sizeof(cliaddr);;
	int n;
	MESSAGE msg;
	//���ܿͻ�����Ϣ
	while(1)
	{
		memset(&msg,0,sizeof(msg));
		
		n=recvfrom(sock,&msg,sizeof(msg),0,(struct sockaddr*)&cliaddr,&clilen);
		if(n<0)
		{
			if(errno==EINTR)
				continue;
			ERR_EXIT("recvfrom");
		}
		
		//�õ���Ϣ������
		int cmd=ntohl(msg.cmd);
		
		switch(cmd)
		{
			//��¼��Ϣ
			case C2S_LOGIN:
				do_login(msg,sock,&cliaddr);
				break;
				//�˳���Ϣ
			case C2S_LOGOUT:
				do_logout(msg,sock,&cliaddr);
				break;
				//�����û��б�
			case C2S_ONLINE_UER:
				do_sendlist(sock,&cliaddr);
				break;
			default:
				break;
			
		}
	}
}
			
			
			
int main()
{

	int sock;
	sock=socket(AF_INET, SOCK_DGRAM,0);
	if(sock==-1)
		ERR_EXIT("socket");
	struct sockaddr_in servaddr;
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_port=htons(8888);
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	
	if(bind(sock,(struct sockaddr *)&servaddr,sizeof(servaddr))<0)
		ERR_EXIT("bind");
	
	chat_srv(sock);
	return 0;
}

//��¼
void do_login(MESSAGE & msg,int sock,struct sockaddr_in * cliaddr)
{
	user_info user;
	//�õ��û���Ϣ
	strcpy(user.username,msg.body);
	user.ip=cliaddr->sin_addr.s_addr;
	user.port=cliaddr->sin_port;
	
	USER_LIST::iterator it;
	
	//���֮ǰ�Ƿ��Ѿ����ڸ��û���
	for(it=client_list.begin();it!=client_list.end();++it)
	{
		if(strcmp(it->username,msg.body)==0)
		{
			break;
		}
	}
	
	//������û���֮ǰ������
	if(it==client_list.end())
	{
		
		//��¼�ɹ�Ӧ��
		printf("has a user login : %s <-> %s:%d\n",msg.body,inet_ntoa(cliaddr->sin_addr),ntohs(cliaddr->sin_port));
		//����û�
		client_list.push_back(user);
		
		MESSAGE reply_msg;
		memset(&reply_msg,0,sizeof(reply_msg));
		reply_msg.cmd=htonl(S2C_LOGIN_OK);
		//���û�������¼�ɹ�����Ϣ
		sendto(sock,&reply_msg,sizeof(reply_msg),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
		
		int count=htonl(client_list.size());
		//������������
		sendto(sock,&count,sizeof(int),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
		
		
		printf("sending user list information to: %s <-> %s:%d\n",msg.body,inet_ntoa(cliaddr->sin_addr),ntohs(cliaddr->sin_port));
		//���Ѿ���¼���û����������û���Ϣ
		for(it=client_list.begin();it!=client_list.end();++it)
		{
			sendto(sock,&*it,sizeof(USER_INFO),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
		}
		//�������û�֪ͨ�����û���¼
		for(it=client_list.begin();it!=client_list.end();++it)
		{
			if(strcmp(it->username,msg.body)==0)
			{
				continue;
			}
			
			//�õ��û��б��е������û��ĵ�ַ
			struct sockaddr_in peeraddr;
			memset(&peeraddr,0,sizeof(peeraddr));
			peeraddr.sin_family=AF_INET;
			peeraddr.sin_port=it->port;
			peeraddr.sin_addr.s_addr=it->ip;
			
			//��Ҫ���͵���Ϣ
			msg.cmd=htonl(S2C_SOMEONE_LOGIN);
			memcpy(msg.body,&user,sizeof(user));
	
			if(sendto(sock,&msg,sizeof(MESSAGE),0,(struct sockaddr*)&peeraddr,sizeof(peeraddr))<0)
				ERR_EXIT("sendto");
		
		}

	}
	else  //˵���û��Ѿ���¼��
	{
		printf("user %s has already logined\n",msg.body);
		MESSAGE reply_msg;
		memset(&reply_msg,0,sizeof(reply_msg));
		reply_msg.cmd=htonl(S2C_ALREADY_LOGINED);
		sendto(sock,&reply_msg,sizeof(MESSAGE),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
		
	}
	
}


//�Լ���д��

void do_logout(MESSAGE & msg,int sock,struct sockaddr_in * cliaddr)
{
	msg.cmd=htonl(S2C_SOMEONE_LOGOUT);
	USER_LIST::iterator it;
	user_info user;
	//�������û�֪ͨ���û��˳�
	for(it=client_list.begin();it!=client_list.end(); )
	{
		if(strcmp(it->username,msg.body)==0)
		{
			
			printf("%s logout\n",it->username);
			it=client_list.erase(it);
			continue;
		}
		
		struct sockaddr_in peeraddr;
		memset(&peeraddr,0,sizeof(peeraddr));
		peeraddr.sin_family=AF_INET;
		peeraddr.sin_port=it->port;
		peeraddr.sin_addr.s_addr=it->ip;
		
		if(sendto(sock,&msg,sizeof(MESSAGE),0,(struct sockaddr*)&peeraddr,sizeof(peeraddr))<0)
			ERR_EXIT("sendto");
		++it;
	}
	
}

//�Լ���д��
void do_sendlist(int sock,struct sockaddr_in * cliaddr)
{
	printf("sendlist\n");
	USER_LIST::iterator it;
	
	MESSAGE reply_msg;
	memset(&reply_msg,0,sizeof(reply_msg));
	reply_msg.cmd=htonl(S2C_ONLINE_USER);
	sendto(sock,&reply_msg,sizeof(reply_msg),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
	
	int count=htonl(client_list.size());
	//������������
	printf("%d\n",ntohs(cliaddr->sin_port));
	sendto(sock,&count,sizeof(int),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
	printf("count=%d\n",client_list.size());
		
	
	//���������û���Ϣ
	for(it=client_list.begin();it!=client_list.end();++it)
	{
		printf("sendto\n");
		sendto(sock,&*it,sizeof(USER_INFO),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
	}
}

















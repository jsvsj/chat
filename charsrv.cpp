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

//用户列表
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
	//接受客户的消息
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
		
		//得到消息的类型
		int cmd=ntohl(msg.cmd);
		
		switch(cmd)
		{
			//登录消息
			case C2S_LOGIN:
				do_login(msg,sock,&cliaddr);
				break;
				//退出消息
			case C2S_LOGOUT:
				do_logout(msg,sock,&cliaddr);
				break;
				//请求用户列表
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

//登录
void do_login(MESSAGE & msg,int sock,struct sockaddr_in * cliaddr)
{
	user_info user;
	//得到用户信息
	strcpy(user.username,msg.body);
	user.ip=cliaddr->sin_addr.s_addr;
	user.port=cliaddr->sin_port;
	
	USER_LIST::iterator it;
	
	//检查之前是否已经存在该用户名
	for(it=client_list.begin();it!=client_list.end();++it)
	{
		if(strcmp(it->username,msg.body)==0)
		{
			break;
		}
	}
	
	//如果该用户名之前不存在
	if(it==client_list.end())
	{
		
		//登录成功应答
		printf("has a user login : %s <-> %s:%d\n",msg.body,inet_ntoa(cliaddr->sin_addr),ntohs(cliaddr->sin_port));
		//添加用户
		client_list.push_back(user);
		
		MESSAGE reply_msg;
		memset(&reply_msg,0,sizeof(reply_msg));
		reply_msg.cmd=htonl(S2C_LOGIN_OK);
		//向用户发生登录成功的消息
		sendto(sock,&reply_msg,sizeof(reply_msg),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
		
		int count=htonl(client_list.size());
		//发送在线人数
		sendto(sock,&count,sizeof(int),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
		
		
		printf("sending user list information to: %s <-> %s:%d\n",msg.body,inet_ntoa(cliaddr->sin_addr),ntohs(cliaddr->sin_port));
		//向已经登录的用户发送在线用户信息
		for(it=client_list.begin();it!=client_list.end();++it)
		{
			sendto(sock,&*it,sizeof(USER_INFO),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
		}
		//向其他用户通知有新用户登录
		for(it=client_list.begin();it!=client_list.end();++it)
		{
			if(strcmp(it->username,msg.body)==0)
			{
				continue;
			}
			
			//得到用户列表中的其他用户的地址
			struct sockaddr_in peeraddr;
			memset(&peeraddr,0,sizeof(peeraddr));
			peeraddr.sin_family=AF_INET;
			peeraddr.sin_port=it->port;
			peeraddr.sin_addr.s_addr=it->ip;
			
			//需要发送的消息
			msg.cmd=htonl(S2C_SOMEONE_LOGIN);
			memcpy(msg.body,&user,sizeof(user));
	
			if(sendto(sock,&msg,sizeof(MESSAGE),0,(struct sockaddr*)&peeraddr,sizeof(peeraddr))<0)
				ERR_EXIT("sendto");
		
		}

	}
	else  //说明用户已经登录了
	{
		printf("user %s has already logined\n",msg.body);
		MESSAGE reply_msg;
		memset(&reply_msg,0,sizeof(reply_msg));
		reply_msg.cmd=htonl(S2C_ALREADY_LOGINED);
		sendto(sock,&reply_msg,sizeof(MESSAGE),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
		
	}
	
}


//自己编写的

void do_logout(MESSAGE & msg,int sock,struct sockaddr_in * cliaddr)
{
	msg.cmd=htonl(S2C_SOMEONE_LOGOUT);
	USER_LIST::iterator it;
	user_info user;
	//向其他用户通知有用户退出
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

//自己编写的
void do_sendlist(int sock,struct sockaddr_in * cliaddr)
{
	printf("sendlist\n");
	USER_LIST::iterator it;
	
	MESSAGE reply_msg;
	memset(&reply_msg,0,sizeof(reply_msg));
	reply_msg.cmd=htonl(S2C_ONLINE_USER);
	sendto(sock,&reply_msg,sizeof(reply_msg),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
	
	int count=htonl(client_list.size());
	//发送在线人数
	printf("%d\n",ntohs(cliaddr->sin_port));
	sendto(sock,&count,sizeof(int),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
	printf("count=%d\n",client_list.size());
		
	
	//发送在线用户信息
	for(it=client_list.begin();it!=client_list.end();++it)
	{
		printf("sendto\n");
		sendto(sock,&*it,sizeof(USER_INFO),0,(struct sockaddr*)cliaddr,sizeof(*cliaddr));
	}
}

















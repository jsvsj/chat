#ifndef _PUB_H_
#define _PUB_H_

#include<list>
#include<algorithm>

using namespace std;

//C2S
//客户到服务器的消息类型
#define C2S_LOGIN 0x01
#define C2S_LOGOUT 0x02
#define C2S_ONLINE_UER 0x03


//消息的数据的最大长度
#define MSG_LEN 512

//S2C
//服务器到用户的消息类型
#define S2C_LOGIN_OK 0x01 
#define S2C_ALREADY_LOGINED 0x02 
#define S2C_SOMEONE_LOGIN 0x03 
#define S2C_SOMEONE_LOGOUT 0x04
#define S2C_ONLINE_USER 0x05


//C2C
//用户到用户的消息类型
#define C2C_CHAT 0x06

//客户端服务器传递的消息格式
typedef struct message
{
	int cmd;
	char body[MSG_LEN];
}MESSAGE;


//用户信息的结构体
typedef struct user_info
{
	char username[16];
	//都是网络字节序
	unsigned int ip;
	unsigned short port;
}USER_INFO;



//用户到用户之间传递的消息
typedef struct chat_msg
{
		char username[16];
		char msg[100];
}CHAT_MSG;


typedef list<USER_INFO> USER_LIST;


#endif


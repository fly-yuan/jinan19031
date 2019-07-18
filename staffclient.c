#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define N 32
#define M 128
#define R 0x1 //注册
#define L 0x2 //登录
#define Q 0x3  //查询单词
#define H 0x4  //历史记录
#define E 0x5 //退出
#define D 0x6 
#define X 0x7
#define USER_Q 0X8
#define NAMELEN 16
#define DATALEN 128

typedef struct staff_info{
	int  no; 			//员工编号
	int  usertype;  	//ADMIN 1	USER 2	 
	char name[NAMELEN];	//姓名
	char pwd[8]; 	//密码
	int  age; 			// 年龄
	
}staff_info_t;

typedef struct{
	int type;//消息类型
	char name[N];//用户名
	char text[M];  //通信的消息
	int  flags;      //标志位
	staff_info_t info; 
}MSG;
#define LEN_SMG sizeof(MSG)
#define err_log(log)\
	do{\
	 perror(log);\
	 exit(1);\
	}while(0)

typedef struct sockaddr SA;
void show_userinfo(MSG *msg);
void do_register(int sockfd,MSG *msg);
int do_login(int sockfd,MSG *msg);
void do_admin_query(int sockfd,MSG *msg);
void do_admin_adduser(int sockfd,MSG *msg);
void do_admin_deluser(int sockfd,MSG *msg);
void do_admin_modification(int sockfd,MSG *msg);
void do_admin_history (int sockfd,MSG *msg);

void do_user_query(int sockfd,MSG *msg);
void do_history(int sockfd,MSG *msg);
int main(int argc, const char *argv[])
{
	
	int sockfd;
	struct sockaddr_in serveraddr;
	socklen_t len=sizeof(SA);
	int cmd;
	char clean[M]={0};
	MSG msg;
	if(argc!=3)
	{
		printf("User:%s <IP> <port>\n",argv[0]);
		return -1;
	}

	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		err_log("fail to socket");
	}
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(atoi(argv[2]));
	serveraddr.sin_addr.s_addr=inet_addr(argv[1]);

	if(connect(sockfd,(SA*)&serveraddr,len)<0)
	{
		err_log("fail to connect");
	}

BEF:
	while(1)//一级的界面
	{
		printf("*************************************************************\n");
		printf("********  1：管理员模式    2：普通用户模式    3：退出********\n");
		printf("*************************************************************\n");
		printf("请输入您的选择（数字）>>");
		 if(scanf("%d",&cmd)!=1)
		 {
				puts("input error");
				fgets(clean,M,stdin);
				continue;
		 }
		 switch(cmd)
		 {
			case 1:
				if(do_login(sockfd,&msg)==1)
				{
					goto ADMIN;
				}
				else
				{
					break;
				}
			case 2:
				if(do_login(sockfd,&msg)==1)
				{
					goto USER;
				}
				else
				{
					break;
				}
			
			case 3:
				goto EXIT;
			default:
				puts("input error");
				break;

		 }
	}

EXIT:  
	
	msg.type=E;
	send(sockfd,&msg,LEN_SMG,0);
	close(sockfd);  //客户端退出的处理   
	exit(0);
//管理员菜单
ADMIN:
	while(1)
	{
		printf("*************************************************************\n");
		printf("* 1：查询  2：修改 3：添加用户  4：删除用户  5：查询历史记录*\n");
		printf("* 6：退出													*\n");
		printf("*************************************************************\n");
		printf("请输入您的选择（数字）>>");
		scanf("%d",&cmd);
		getchar();

		switch(cmd)
		{
			case 1:
			 	do_admin_query(sockfd,&msg);
			 	break;
			case 2:
			 	do_admin_modification(sockfd,&msg);
			 	break;
			case 3:
			 	do_admin_adduser(sockfd,&msg);
			 	break;
			case 4:
			 	do_admin_deluser(sockfd,&msg);
			 	break;
			case 5:
			 	do_admin_history(sockfd,&msg);
			 	break;
			 case 6:
			 	msg.type =E;
			 
			 	send(sockfd, &msg, sizeof(MSG), 0);
			 	close(sockfd);
			 	exit(0);
			 default:
			 	printf("您输入有误，请重新输入！\n");
		}
	}


USER:
	while(1)//二级界面
	{
		printf("*************************************************************\n");
		printf("*************  1：查询  	2：修改		3：退出	 *************\n");
		printf("*************************************************************\n");
		printf("请输入您的选择（数字）>>");
		if(scanf("%d",&cmd)!=1)
		 {
				puts("input error");
				fgets(clean,M,stdin);
				continue;
		 }
		 switch(cmd)
		 {
		    case 1:
                 do_user_query(sockfd,&msg);
				 break;
			case 2:
				 do_admin_modification(sockfd,&msg);
				 break;
			case 3:
				goto BEF;
				break;
			default:
				puts("cmd error");
				break;
		 }

	}

}

void do_register(int sockfd,MSG *msg)
{
	msg->type=R;
	puts("input name>>>");
	scanf("%s",msg->name);
	puts("input pwd>>>");
	scanf("%s",msg->text);

	send(sockfd,msg,LEN_SMG,0);
	recv(sockfd,msg,LEN_SMG,0);
	if(strncmp(msg->text,"OK",2)==0)
	{
		puts("Register ok!");
		return;
	}
	else
	{
		puts("Register fail!");
		return;
		
	}
}
int do_login(int sockfd,MSG *msg)
{
	msg->type=L;
	puts("请输入用户名>>>");
	scanf("%s",msg->name);
	puts("请输入密码>>>");
	scanf("%s",msg->text);

	send(sockfd,msg,LEN_SMG,0);
	recv(sockfd,msg,LEN_SMG,0);
	if(strncmp(msg->text,"OK",2)==0)
	{
		 puts("Login ok!");
		 return 1;
	}
	else
	{
		puts("Login fail!");
		return -1;
	}
}
void show_userinfo(MSG *msg)
{
	puts("======================================================================================");	
	printf("%s.\n",msg->text);
	return;
}

void do_admin_query(int sockfd,MSG *msg)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	msg->type=Q;
	int i=0;
	while(1)
	{
		memset(&msg->info,0,sizeof(staff_info_t));
		if(msg->type == Q)
		{
			printf("*************************************************************\n");
			printf("******* 1：按人名查找  	2：查找所有 	3：退出	 *******\n");
			printf("*************************************************************\n");
			printf("请输入您的选择（数字）>>");
			scanf("%d",&i);
			getchar();

			switch(i)
			{
				case 1:
					msg->flags = 1;//按人名查找
					break;
				case 2:
					msg->flags = 0;//默认查找所有员工
					break;
				case 3:
					return;
			}	
		}


		if(msg->flags == 1)
		{
			printf("请输入您要查找的用户名：");
			scanf("%s",msg->info.name);
			getchar();
			
			send(sockfd, msg, sizeof(MSG), 0);
			recv(sockfd, msg, sizeof(MSG), 0);
			printf("工号\t 姓名\t密码\t年龄\n");
			show_userinfo(msg);

		}else{
			send(sockfd, msg, sizeof(MSG), 0);
			printf("工号\t姓名\t密码\t年龄\n");
			while (1)
			{	
				//循环接受服务器发送的用户数据
				recv(sockfd, msg, sizeof(MSG), 0);
				if(strncmp(msg->text , "over*",5) ==0)
					break;
				show_userinfo(msg);
			}
		}
	}	
	printf("查找结束\n");	
}
void do_admin_adduser(int sockfd,MSG *msg)//管理员添加用户
{		
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	//输入用户信息--填充封装命令请求---发送数据---等待服务器响应---成功之后自动查询当前添加的用户
	char temp;
	msg->type  = R;
	memset(&msg->info,0,sizeof(staff_info_t));
	
	while(1){	
		printf("***************热烈欢迎新员工***************.\n");
		printf("请输入工号：");
		scanf("%d",&msg->info.no); 
		getchar();
		printf("您输入的工号是：%d\n",msg->info.no);
		printf("工号信息一旦录入无法更改，请确认您所输入的是否正确！(Y/N)");
		scanf("%c",&temp);
		getchar();
		if(temp == 'N' || temp == 'n'){
			printf("请重新添加用户：");
			break;
		}

		printf("请输入用户名：");
		scanf("%s",msg->info.name);
		getchar();
		
		printf("请输入用户密码：");
		scanf("%6s",msg->info.pwd);
		getchar();
		
		printf("请输入年龄：");
		scanf("%d",&msg->info.age);
		getchar();

		//发送用户数据
		send(sockfd, msg, sizeof(MSG), 0);
		//等待接收服务器响应
		recv(sockfd, msg, sizeof(MSG), 0);
		
		if(strncmp(msg->text,"OK",2) == 0)
			printf("添加成功！\n");
		else
			printf("%s",msg->text);
		
		printf("是否继续添加员工:(Y/N)");
		scanf("%c",&temp);
		getchar();
		if(temp == 'N' || temp == 'n')
			break;
	}

}
void do_admin_deluser(int sockfd,MSG *msg)//管理员删除用户
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	//输入要删除的用户名和密码--封装命令请求---发送数据---等待服务器响应
	
	msg->type = D;
	
	printf("请输入要删除的用户工号："); //工号唯一
	scanf("%d",&msg->info.no);
	getchar();
	printf("请输入要删除的用户名："); //工号唯一
	scanf("%s",msg->info.name);
	getchar();

	send(sockfd, msg, sizeof(MSG), 0);
	recv(sockfd, msg, sizeof(MSG), 0);
	
	if(strncmp(msg->text,"OK",2)==0)
		printf("删除成功！\n");
	else
		printf("%s",msg->text);

	printf("删除工号为：%d 的用户.\n",msg->info.no);

}
void do_admin_modification(int sockfd,MSG *msg)//管理员修改
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	int n = 0;
	msg->type = X;
	memset(&msg->info,0,sizeof(staff_info_t));

	send(sockfd, msg, sizeof(MSG), 0);
	recv(sockfd, msg, sizeof(MSG), 0);

	printf("请输入您要修改的工号：");
	scanf("%d", &msg->info.no);
	getchar();
	
	printf("*******************请输入要修改的选项********************\n");
	printf("******	1：姓名	  2：年龄	3：密码	 10：退出  ******\n");
	printf("*************************************************************\n");
	printf("请输入您的选择（数字）>>");
	scanf("%d",&n);
	getchar();
	
	switch(n)
	{
		case 1:
			printf("请输入用户名：");
			msg->text[0] = 'N';
			scanf("%s",msg->info.name);getchar();
			break;
		case 2:
			printf("请输入年龄：");
			msg->text[0] = 'A';
			scanf("%d",&msg->info.age);getchar();
			break;
		
		case 3:
			printf("请输入新密码：(6位数字)");
			msg->text[0] = 'M';
			scanf("%6s",msg->info.pwd);getchar();
			break;
		case 10:
			return ;
	}
	
	send(sockfd, msg, sizeof(MSG), 0);
	recv(sockfd, msg, sizeof(MSG), 0);
	printf("%s",msg->text);

	printf("修改结束.\n");

}
void do_user_query(int sockfd,MSG *msg)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	msg->type = USER_Q;
	
	//只能查询自己的信息，所以不需要重新输入用户名，直接send请求就可以
	send(sockfd, msg, sizeof(MSG), 0);
	recv(sockfd, msg, sizeof(MSG), 0);
	printf("工号\t 姓名\t密码\t年龄\t\n");
	show_userinfo(msg);
}
void do_history(int sockfd,MSG *msg){}
void do_admin_history (int sockfd,MSG *msg)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	//封装命令请求---发送数据---等待服务器响应----打印输出结果
		
	msg->type = H;
	send(sockfd, msg, sizeof(MSG), 0);
	
	while(1){
		recv(sockfd, msg, sizeof(MSG), 0);
		if(strncmp(msg->text ,"over*",5) ==0)
			break;	
		printf("msg->text: %s",msg->text);
	}
	printf("admin查询历史记录结束！\n");
	
}
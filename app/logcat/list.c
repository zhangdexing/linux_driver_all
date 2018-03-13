#include "list.h"

/*链表初始化函数
 *功能：给链表初始化并分配空间
 *参数：无
 *返回值：head：已分配空间的头结点
 *		  NULL：分配失败
*/
file_list *list_init()
{
	//分配内存空间
	file_list *head = (file_list *)malloc(sizeof(file_list));
	
	//判断分配是否成功
	if(head == NULL)
	{
		perror("memory allocation failed!");
		return NULL;
	}

	return head;
}

/*链表尾插函数
 *功能：将传过来的字符串插入链表的最后
 *参数：head：链表的头结点
 *		temp：即将插入链表的字符串
 *返回值：-1：链表的头结点为空
 *		  -2：给新建节点分配空间失败
 *		  0： 数据插入成功
*/

int list_insert(file_list *head, char *temp)
{
	/*判断头结点是否配空*/
	if(head == NULL)
	{
		perror("Head node is empty!");
		return -1;
	}
	
	file_list *p = head;
	
	/*新建一个节点*/
	file_list *newlist = (file_list *)malloc(sizeof(file_list));
	newlist->datebuf = (char *)malloc(SIZE);
	
	if(newlist == NULL)
	{
		perror("memory allocation failed!");
		return -2;
	}

	strcpy(newlist->datebuf,temp);
	
	/*遍历到最后一个节点*/
	while(p->next != NULL)
		p = p->next;
	
	/*将新节点插入链表最后*/
	p->next = newlist;
	
	return 0;
}

void list_display(file_list *head)
{
	file_list *p = head;
	if(head == NULL)
	{
		perror("Head node is empty!");
		return ;
	}
	
	//不打印头结点
	while(p->next != NULL)
	{
		printf("datebuf is %s", p->next->datebuf);
		p = p->next;
	}
}

bool list_lookup(file_list *head, const char *temp)
{
	if(head == NULL)
	{
		perror("Head node is empty!");
		return false;
	}
	
	file_list *p = head;
	
	//遍历比较
	while(p->next != NULL)
	{
		/*去掉字符串后面的\r\n*/
		int i = 0;
		char *t = (char *)malloc(SIZE);
		strcpy(t, p->next->datebuf);
		
		//现将指针执行字符创的末尾
		while(*t != '\0')
			t++;	
		t--;

		//从后面往前遍历，去掉\r\n
		while(strlen(t) != 0)
		{	
			if(*t == '\r' || *t == '\n')
				*t = '\0';	
			t--;	
		}	
		t++;	
		
		//相等就退出
		if(strcmp(t,temp) == 0)
		{
			//printf("find date %s\n", p->next->datebuf);
			break;
		}	
		p = p->next;
	}
	
	if(p->next == NULL)
		return false;
	
	return true;
}

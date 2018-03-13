#ifndef __LIST_H
#define __LIST_H

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define SIZE 1024

//存放数据的单链表
typedef struct list
{
	char *datebuf;
	struct list *next;

}file_list; 


file_list *list_init();
int list_insert(file_list *head, char *temp);
void list_display(file_list *head);
bool list_lookup(file_list *head,  const char *temp);
                                                                                                                    
#endif
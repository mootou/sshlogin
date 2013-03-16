/*
 *	Jiang Wenxu (), jwx0819@gmail.com
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#define CONF           "/home/.sshlogininfo"
#define HOSTLIST       "/home/.hostlist"
#define AUTO_ADD_HOST_KEY \
	"-oUserKnownHostsFile=/dev/null -oStrictHostKeyChecking=no"
#define USER           "test"
#define PASSWORD       "12345"
#define BUF_SIZE       512
#define MAX_BUF_SIZE   4096

struct record
{
	char group[128];
	char address[128];
	char note[128];

	struct record *next;
} *r_head, *r_tail, *s_head, *s_tail;

void usage()
{
	printf("\n"
			"show all servers:    sshlogin -s\n"
			"login a server:      sshlogin -c test\n"
			"add a server:        sshlogin -a test 1.1.1.1 phy\n"
			"add servers:         sshlogin -A test 1.1.1 1 3 phy\n"
			"scp file to remote:  sshlogin -t 2.2.2.2 /local/file\n"
			"scp file to local:   sshlogin -r 2.2.2.2 /remote/file /local/\n"
			"scp file to group:   sshlogin -T (test|all) /local/file\n"
			"ssh a cmd in remote: sshlogin -x 2.2.2.2 \"ls /data1\"\n"
			"ssh a cmd in group:  sshlogin -X (test|all) \"ls /data1\"\n"
			"\n"
			"warning: To save the parameters correctly, donot input '|'\n\n"
			);
}

int s_arg = 0;
int a_arg = 0;
int A_arg = 0;
int c_arg = 0;
int t_arg = 0;
int T_arg = 0;
int x_arg = 0;
int X_arg = 0;
int r_arg = 0;

int thread_num = 0;
pthread_mutex_t mutex;

int opt_get(int argc, char **argv)
{
	int ch;

	if ((ch = getopt(argc, argv, "?saAchtrTxX")) > 0)
	{
		switch (ch)
		{
			case '?':
				usage();
				exit(0);

			case 'h':
				usage();
				exit(0);

			case 's':
				s_arg = 1;
				break;

			case 'c':
				c_arg = 1;
				break;

			case 'a':
				a_arg = 1;
				break;

			case 'A':
				A_arg = 1;
				break;

			case 't':
				t_arg = 1;
				break;

			case 'r':
				r_arg = 1;
				break;

			case 'T':
				T_arg = 1;
				break;

			case 'x':
				x_arg = 1;
				break;

			case 'X':
				X_arg = 1;
				break;
		}
	}

	return 0;
}

int write_record(char *group, char *address, char *note)
{
	FILE *fp;
	char *buf = calloc(strlen(group) + 
			strlen(address) + strlen(note) + 10, 1);
	sprintf(buf, "%s|%s|%s\n", group, address, note);

	fp = fopen(CONF, "a");
	fwrite(buf, strlen(buf), 1, fp);
	fclose(fp);

	return 0;
}

#define GET_INFO(line, g, a, n) do{\
	char *_p = strchr(line, '|');\
	memcpy(g, line, _p - line);\
	line = _p + 1;\
	_p = strchr(line, '|');\
	memcpy(a, line, _p - line);\
	line = _p + 1;\
	_p = strpbrk(line, "\r\n");\
	memcpy(n, line, _p - line);\
} while (0)

int record_list(char *group)
{
	FILE *fp;
	char *p, read_buf[BUF_SIZE];

	if (!(fp = fopen(CONF, "r")))
	{
		printf("cannot open configure\n");
		exit(0);
	}

	memset(read_buf, 0, BUF_SIZE);

	while (fgets(read_buf, BUF_SIZE, fp))
	{
		struct record *r = calloc(sizeof(struct record), 1);

		p = read_buf;
		GET_INFO(p, r->group, r->address, r->note);

		if ((group && !strcmp(group, r->group)) || !group)
		{
			if (!r_head)
			{
				r_head = r_tail = r;
			}
			else
			{
				r_tail->next = r;
				r_tail = r;
			}
		}
		else
		{
			free(r);
		}

		memset(read_buf, 0, BUF_SIZE);
	}

	fclose(fp);

	return 0;
}

void show_list()
{
	int i = 0;
	struct record *r = r_head;

	printf("num   \taddress    \tgroup\tnote\n");
	while (r)
	{
		printf("%d\t%s\t%s\t%s\n", ++i, r->address, r->group, r->note);
		r = r->next;
	}
}

void select_record()
{
	int i, j = 0;
	struct record *r = r_head;

	printf("choose : ");
	scanf("%d", &i);

	while (r)
	{
		if (++j == i)
		{
			struct record *s = calloc(sizeof(struct record), 1);
			strcpy(s->address, r->address);
			if (!s_head)
			{
				s_head = s_tail = s;
			}
			else
			{
				s_tail->next = s;
				s_tail = s;
			}

			return;
		}

		r = r->next;
	}
}

char *system_exec(char *cmd)
{
	char *result = calloc(MAX_BUF_SIZE, 1);
	int result_n = 0;
	FILE *pp;
	
	if ((pp = popen(cmd, "r")) == NULL)
	{
		printf("[system_exec] popen() error");
		free(result);
		return NULL;
	}
	
	while (fgets(result + result_n, MAX_BUF_SIZE, pp))
	{
		result_n = strlen(result);
		result = realloc(result, result_n + MAX_BUF_SIZE + 1);
		memset(result + result_n, 0, MAX_BUF_SIZE + 1);
	}

	result = realloc(result, result_n + 1);
	result[result_n] = 0;

	pclose(pp);

	return result;
}

void add_s_list(char *address)
{
	if (address)
	{
		struct record *s = calloc(sizeof(struct record), 1);
		strcpy(s->address, address);

		if (!s_head)
		{
			s_head = s_tail = s;
		}
		else
		{
			s_tail->next = s;
			s_tail = s;
		}
	}
	else
	{
		s_head = r_head;
	}
}

struct exe_s
{
	char ip[32];
	char cmd[1024];
};

void *thread_exec(void *data)
{
	char *p;
	struct exe_s *e = (struct exe_s *)data;

	if ((p = system_exec(e->cmd)) && *p)
	{
		printf("%s : \n%s\n", e->ip, p);
	}
	else
	{
		printf("%s : \n    success\n\n", e->ip);
	}

	pthread_mutex_lock(&mutex);
	thread_num--;
	pthread_mutex_unlock(&mutex);

	pthread_exit(0);
}

void sshcmd_exec(char *cmd)
{
	struct record *s = s_head;
	char buf[1024];

	sprintf(buf, ">%s", HOSTLIST);
	system(buf);

	while (s)
	{
		sprintf(buf, "echo \"%s\" >> %s", s->address, HOSTLIST);
		system(buf);
		s = s->next;
	}

	sprintf(buf, "sshpt -f %s -u %s -s -P %s \"%s\"", 
			 HOSTLIST, USER, PASSWORD, cmd);
	system(buf);
}

void scp_exec(char *file)
{
	struct record *s = s_head;
	int num;

	while (s)
	{
		struct exe_s *e = calloc(sizeof(struct exe_s), 1);
		sprintf(e->cmd, "sshpass -p \"%s\" scp %s -r %s %s@%s:/home/%s/", 
				PASSWORD, AUTO_ADD_HOST_KEY, file, USER, s->address, USER);
		sprintf(e->ip, s->address);

		pthread_t thread;
		pthread_create(&thread, NULL, thread_exec, e);
		pthread_detach(thread);

		pthread_mutex_lock(&mutex);
		thread_num++;
		pthread_mutex_unlock(&mutex);

		s = s->next;
	}

	while(1)
	{
		sleep(2);

		pthread_mutex_lock(&mutex);
		num = thread_num;
		pthread_mutex_unlock(&mutex);

		if (!num) break;
	}
}

void scp_r_exec(char *file, char *local)
{
	char *p;
	char command[BUF_SIZE];

	sprintf(command, "sshpass -p \"%s\" scp %s -r %s@%s:%s %s", 
			PASSWORD, AUTO_ADD_HOST_KEY, USER, s_head->address, file, local);
	if ((p = system_exec(command)) && *p)
	{
		printf("%s : \n%s\n", s_head->address, p);
	}
	else
	{
		printf("%s : \n    success\n\n", s_head->address);
	}
}

void ssh_exec()
{
	char command[BUF_SIZE];
	sprintf(command, "sshpass -p \"%s\" ssh %s@%s %s", 
			PASSWORD, USER, s_head->address, AUTO_ADD_HOST_KEY);
	system(command);
}

int main(int argc, char **argv)
{
	int i;
	char buf[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);

	pthread_mutex_init(&mutex, NULL);

	opt_get(argc, argv);
	r_head = r_tail = NULL;
	s_head = s_tail = NULL;

	if (s_arg == 1)
	{
		record_list(NULL);
		show_list();
		exit(0);
	}

	if (c_arg == 1)
	{
		record_list(argv[2]);
		show_list();
		select_record();
		ssh_exec();
		exit(0);
	}

	if (a_arg == 1)
	{
		if (!argv[2] || !argv[3] || !argv[4])
		{
			printf("input error\n");
			usage();
			exit(0);
		}

		write_record(argv[2], argv[3], argv[4]);
		exit(0);
	}

	if (A_arg == 1)
	{
		if (!argv[2] || !argv[3] || !argv[4] || !argv[5] || !argv[6])
		{
			printf("input error\n");
			usage();
			exit(0);
		}

		for (i = atoi(argv[4]); i <= atoi(argv[5]); i++)
		{
			sprintf(buf, "%s.%d", argv[3], i);
			write_record(argv[2], buf, argv[6]);
			memset(buf, 0, BUF_SIZE);
		}
		exit(0);
	}

	if (t_arg == 1)
	{
		add_s_list(argv[2]);
		scp_exec(argv[3]);

		exit(0);
	}

	if (r_arg == 1)
	{
		add_s_list(argv[2]);
		scp_r_exec(argv[3], argv[4]);

		exit(0);
	}

	if (T_arg == 1)
	{
		if (!strcmp(argv[2], "all"))
		{
			record_list(NULL);
		}
		else
		{
			record_list(argv[2]);
		}
		add_s_list(NULL);
		scp_exec(argv[3]);

		exit(0);
	}

	if (x_arg == 1)
	{
		add_s_list(argv[2]);
		sshcmd_exec(argv[3]);

		exit(0);
	}

	if (X_arg == 1)
	{
		if (!strcmp(argv[2], "all"))
		{
			record_list(NULL);
		}
		else
		{
			record_list(argv[2]);
		}
		add_s_list(NULL);
		sshcmd_exec(argv[3]);

		exit(0);
	}

	record_list(NULL);
	select_record();
	ssh_exec();

	return 0;
}

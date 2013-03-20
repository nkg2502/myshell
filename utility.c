/*
 * 필요한 것
 * fork_exec에서 exit에 들어갈 정수를 정할것
 *
 * stdin 파일을 대체할 파일을 열지 못한 경우
 * exit status = 1
 * stdout 파일을 대체할 파일을 열지 못한 경우
 * exit status = 2
 * exec 실행에 실패한 경우
 * exit status = 127	<-- system function의 정의 부분에 이렇게 기술
 *
 * 하지만 exit 코드를 확인하지는 않을 것
 *
 * 그리고 myshell.h에 저 위에 있는 코드가 enum 으로 정의될 것임
 *
 * 
 * 좀더 정확한 에러에 관한 status가 필요하다면
 * errno.h 에 관련된 문항을 libc.pdf에서 참고하기로 한다
 *
 *
 */ 

#include "myshell.h"

// 리턴 값에 대해서 더 생각해 볼 것
int fork_exec(char *cmd[], int file_append, int background)
{
	pid_t pid;
	int status; // exit status 

	// 만약에 파일이 존재하지 않거나
	// 사용가능하지 않을때는 어떻게 처리할까?
	// 사용가능 하지 않을때 bash 쉘은 그냥 멈춰버림
	
	// exit() 안에 어떤 값을 넣을지는 고민해봅시다
	switch(pid = fork()) {
		case -1:
			fprintf(stderr, "fork error : %s\n", cmd[CMD_RUN]);
			status = -1;
			break;
		case 0:// 아래의 코드를 좀더 줄여볼 수 잇을듯.. 
			if(cmd[CMD_STDIN]) {
				//pdebug("stdin replaced");
				if(!freopen(cmd[CMD_STDIN], "r", stdin)) {
					//pdebug("file cannot open for stdin");
					exit(STDIN_FAIL);
					// 0말고 다른 숫자 넣으면 그걸로 wait에서 받는다.
					//access가 안됬다... 원래는 그냥 끝나는거임
				}
			}

			if(cmd[CMD_STDOUT]) {
				//pdebug("stdout replaced");
				if(!freopen(cmd[CMD_STDOUT], (file_append ? "a" : "w"), stdout))
					exit(STDOUT_FAIL);
			}

			// 2번 문항
			setenv("PARENT", getenv("SHELL"), 1);
			
			execvp(cmd[CMD_RUN], cmd + CMD_RUN);

			fprintf(stderr, "%s : Command Not Found!\n", cmd[CMD_RUN]);
			exit(EXEC_FAIL); // 책에 그렇게 써있던데? ㅋㅋ
			// exit 대신에 _exit를 부를 수도 있다 
			// 스티븐슨책 p.222
			// Note that we call _exit instead of exit.
			// This is to prevent any standard I/O buffers
			// from being flushed in the child
		default:
			if(!background)
				waitpid(pid, &status, WUNTRACED);
	}

	return status;
}

char** line_parser(char *line, int *file_append, int *background)
{
	static char *cmd[MAX_ARGS];
	char *token;		// token
	int arg_counter = 0;	// argument counter

	// initialize redirection
	cmd[CMD_STDIN] = NULL;
	cmd[CMD_STDOUT] = NULL;

	token = strtok(line, SEPARATORS);

	// strcmp에서 NULL과의 비교는 정의되지 않는군
	// 고육책으로 token을 먼저 비교하게 함
	do {
		if(token) {
			if (!strcmp(token, "<")) {
				cmd[CMD_STDIN] = strtok(NULL, SEPARATORS);
				continue;
			}

			// short circuit evaluation
			if (!strcmp(token, ">") || !strcmp(token, ">>")) {
				cmd[CMD_STDOUT] = strtok(NULL, SEPARATORS);
				if(!strcmp(token, ">>"))
					*file_append = ON;
				continue;
			}

			if (!strcmp(token, "&")) {
				*background = ON;
				continue;
			}
		}

		cmd[CMD_RUN + arg_counter++] = token;

	} while ((token = strtok(NULL, SEPARATORS)));

	// 아래의 문장을 사용하지 않아서 덕분에 오랜 시간 삽질했다 ㅋㅋ
	// 하지만 오히려 운이 좋은 경우라고 해야겠다
	cmd[CMD_RUN + arg_counter] = NULL; // 이거 안하면 파싱이 제대로 안되네.. ;; 
	
	return cmd;
}

int internal_cmd(char *name)
{
	if(!strcmp(name, "cd"))
		return i_cd;
	if(!strcmp(name, "clr"))
		return i_clr;
	if(!strcmp(name, "pause"))
		return i_pause;
	if(!strcmp(name, "quit"))
		return i_quit;

	if(!strcmp(name, "environ"))
		return i_environ;
	if(!strcmp(name, "echo"))
		return i_echo;

	// break 하는 i_cmd
	if(!strcmp(name, "myshell"))
		return i_myshell;
	if(!strcmp(name, "help"))
		return i_help;
	if(!strcmp(name, "dir"))
		return i_dir;

	return i_not;
}

void pstatus(char *cmd[], FILE *fp, int fa, int bg)
{
	char **iter = cmd;
	int counter = 2;
	iter += CMD_RUN;
	printf("==================== status ====================\n");
	printf("stdin\t: %s\n", cmd[CMD_STDIN] == NULL ? "stdin" : cmd[CMD_STDIN]);
	printf("stdout\t: %s,\t(%s[%p])\n", cmd[CMD_STDOUT] == NULL ? "stdout" : cmd[CMD_STDOUT], fp == stdout ? "stdout" : cmd[CMD_STDOUT], fp );
	printf("file\t: %s,\t%sground\n", fa == ON ? "append (>>)" : "write (>)", bg == ON ? "back" : "fore");
	while(*iter) {
		printf("cmd[%02d] : %s [%p]\n", counter++, *iter ? *iter : "NULL", *iter);
		iter++;
	}
	printf("==================== status ====================\n");

}	

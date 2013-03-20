#include "myshell.h"

/*
 * myshell 프로그램은 myshell binary 파일이 있는 곳에서
 * 실행되어야 합니다.
 * 그렇지 않으면 쉘의 작동을 보장할 수 없습니다.
 * 물론 이것이 프로그램의 단점이 될 수 없습니다.
 * 실제 bash 쉘은 path가 걸려 있는 /bin 아래에 존재한다는
 * 것을 가정하고 있습니다.
 * 그렇지만 myshell의 경우에는 /bin 아래에 존재한다는
 * 보장을 할 수 없습니다.
 * 따라서 myshell이 최초로 실행되는 것은
 * myshell의 binary가 존재하는 present working directory에서
 * 실행해야만 모든 것을 보장할 수 있습니다.
 *
 * dir과 help를 위한 alias의 인덱스 접근을 좀더 개선해볼 수 있습니다.
 *
 * 파싱을 할 경우에는
 * strtok 함수에 의지하고 있으므로
 * line_parser 함수 밖에서 strtok 함수의 호출은
 * 제대로된 프로그램의 작동을 보장하지 않습니다.
 * strtok의 설계의 문제이므로 어쩔 수 없습니다.
 * (그렇기 때문에 alias라는 공간을 선언해 둔 것입니다.
 *  마찬가지로 path_readme도 선언되어 있습니다.)
 *
 * flag들은 int형으로 선언했습니다.
 * 물론 C99에서 도입된 boolean 타입인 _Bool을 활용할 수도 있지만
 * 활용하지 않았습니다.
 *
 * internal command의 리다이렉션은 freopen을 사용하지 않았습니다.
 * 왜냐하면 freopen의 경우 해당 스트림을 close해버리므로
 * 문제가 생깁니다.
 * 따라서 FILE *fp를 이용합니다.
 *
 * batchfile을 입력하는 방법은 
 * myshell batchfile 뿐입니다.
 * 추가적으로 myshell batchfile > out, myshell batchfile >> out
 * 까지 보장할 수 있습니다.
 * (myshell batchfile & 도 보장할 수 있습니다.)
 * myshell < batchfile은 실행은 보장됩니다.
 * 단 prompt 가 나타나서 정상적이지 않은 페이지로 보이게됩니다.
 * 총 이상한 방법으로 실행하는 경우는 약 7가지 입니다.
 * 모든 경우에 대해서 보장되는지 확인하지 않았습니다.
 * 위에 설명된 경우는 실행을 보장합니다.
 * (레이스 컨디션과 같은 문제가 발생할 수도 있습니다.
 *  하지만 과제의 범위를 넘어서므로 고려하지 않습니다.) 
 * batchfile안에 internal command인 pause를 사용할 경우
 * 작동에 대한 일관성을 보장할 수 없습니다.
 * (확인된 바에 의하면..
 *  myshell batchfile의 경우에는 pause가 작동하지 않으며
 *  myshell batchfile & 의 경우에는 작동합니다.
 *  하지만 이런 경우를 고려한 batchfile의 작성은
 *  바람직하지 않습니다. [일관성이 보장되지 않습니다])
 *
 * 몇몇 디버깅을 위한 매크로와 함수는 삭제해도 문제가 없습니다.
 *
 */ 

int main(int argc, char *argv[], char *envp[])
{
	char line_buffer[MAX_BUFFER]; // 사용자로부터 입력을 받음
	char **command;		// parsing된 line에 대한 포인터
	int flag_fa;		// file append에 대한 플래그
	int flag_bg;		// background excution에 대한 플래그
	FILE *fp = stdout;	// redirection file pointer for internal command

	// ls -al을 저장할 공간과
	// less /<fullpath>/readme를 저장할 공간이 필요해
	char *alias[] = {
		"ls",
		"-al",
		"less"
	};

	char path_readme[MAX_BUFFER]; // fullpath를 저장할 공간 readme 파일을 위한 것임

	// 쉘을 실행하는 방법은 3가지
	// 1. myshell이 있는 디렉토리에서 path없이 실행
	// 	ex) ./myshell
	// 2. 외부 디렉토리에서 myshell을 경로로 실행
	// 	ex) /home/test/myshell
	// 3. myshell이 있는 디렉토리가 path가 있어서 실행
	// 	ex) myshell
	// 	이런 경우에는 executable한 path를 어떻게 알아내냐?
	// 아무래도 생각하기에는
	// 1번일 가능성이 높은 것으로 사료됨
	// 아래의 코드는 1번을 고려해서 작성
	strncpy(line_buffer, getcwd(NULL, 0), MAX_BUFFER); // SHELL
	strncpy(path_readme, line_buffer, MAX_BUFFER); // readme

	strncat(line_buffer, "/myshell", MAX_BUFFER); // SHELL
	strncat(path_readme, "/readme", MAX_BUFFER); // readme

	setenv("SHELL", line_buffer, 1); // 1. ix.

	if(argv[1]) // for batchfile
		if(!freopen(argv[1], "r", stdin))
			exit(STDIN_FAIL);

	while(!feof(stdin)) {

		if(argc < 2) // batchfile에서는 프롬프트 안나오게 하기 위해서
			printf("%s>", getcwd(NULL, 0));

		if(fgets(line_buffer, MAX_BUFFER, stdin)) {

			flag_fa = OFF;
			flag_bg = OFF;

			if(fp != stdout)
				fclose(fp);
			
			command = line_parser(line_buffer, &flag_fa, &flag_bg);

			// 이쪽 아래로는 line_buffer를
			// strcpy나 strcat 용도로 사용해도 괜찮음
		
			if(command[CMD_RUN]) {

				// internal command alias
				// 왜 내부 커맨드 + .. 하면 디렉토리가 변경되는거지? // 파싱을 잘못해섴
				// NULL을 마지막에 넣어주지 않았기 때문임
				
				// 내부 커맨드의 리다이렉션을 위한 파일 오픈
				if(NULL == (fp = fopen(command[CMD_STDOUT], flag_fa ? "a" : "w")))
					fp = stdout;

				//pstatus(command, fp, flag_fa, flag_bg);
	
				switch(internal_cmd(command[CMD_RUN])) {
					case i_cd: // OLDPWD도 갱신해야하나? 우선은 갱신해놓기로 함
						{
							// 물론 아래의 선언은 필요하지 않다
							// 그렇지만 일관성 측면에서는 더 좋은거 같기도 하다
							char *target_dir = command[CMD_RUN + 1];
							
							if(target_dir)
								if(chdir(target_dir))
									printf("%s : No such directory\n", target_dir);
								else {
									setenv("OLDPWD", getenv("PWD"), 1);
									setenv("PWD", getcwd(NULL, 0), 1);
								}
							else
								printf("%s\n", getcwd(NULL, 0));
						} continue;
					case i_dir:
						// internal command and alias이기 때문에
						// ls -al로 실행하면 될꺼 같다
						// 문제는 뭐냐면 ls a b c 이런것도 가능하다는거다...
						// arg는 최대 64개를 넘지 않는다고 가정한다
						{
							int arg_counter = CMD_RUN;
							while(command[arg_counter++])
								;
							while(arg_counter-- > CMD_RUN + 2)
								command[arg_counter] = command[arg_counter - 1];

							command[CMD_RUN] = alias[0]; // ls
							command[CMD_RUN + 1] = alias[1]; // -al

							//pstatus(command, fp, flag_fa, flag_bg);

						} break; 
					case i_environ:
						{
							char **env = envp;
							while(*env)
								fprintf(fp, "%s\n", *env++);
							fprintf(fp, "\n");
						} continue;
					case i_clr:
						{
							fprintf(stdout, "[H[2J");
							fflush(stdout);
						} continue;
					case i_echo:
						{
							char **comment = command + CMD_RUN;
							while(*++comment)
								fprintf(fp, "%s ", *comment);
							fprintf(fp, "\n");
						} continue;
					case i_help:
						{
							// index 접근법을 좀 바꿀까?
							command[CMD_RUN] = alias[2]; // less
							command[CMD_RUN + 1] = path_readme;
							command[CMD_RUN + 2] = NULL;
						} break;
					case i_pause:
						{
							pdebug("pause");
							struct termios term;
							// echo off
							tcgetattr(STDIN_FILENO, &term);
							term.c_lflag ^= ECHO;
							tcsetattr(STDIN_FILENO, TCSANOW, &term);

							printf("Press Enter to continue...\n");
							fgets(line_buffer, MAX_BUFFER, stdin);

							// echo on
							tcgetattr(STDIN_FILENO, &term); // 이건 없어도 작동할듯?
							term.c_lflag ^= ECHO;
							tcsetattr(STDIN_FILENO, TCSANOW, &term);

						} continue;
					case i_myshell: // for batchfile
						{
							command[CMD_RUN] = getenv("SHELL");
						} break;
					case i_quit:
						exit(0);
				}

				fork_exec(command, flag_fa, flag_bg);
			}
		}
	}

	exit(0);
	// return 0도 물론 가능하지만 
	// forking과 exec를 하므로
	// 좀더 안전하지 않을까?
}

/* Copyright (C) 1991-2012 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
 
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
 
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
 
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <readline/readline.h>

#define JUMP_CODE 	(55)
#define MAX_PARAMS 	(8)

static volatile sig_atomic_t jump_active = 0;
static sigjmp_buf env;

void sigint_hndlr(int signo)
{
	if (!jump_active)
		return;

	siglongjmp(env, JUMP_CODE);
}

int cd(char *path)
{
	return chdir(path);
}

char **get_input(char *input)
{
	const char *sep = " ";
	char **command = malloc(MAX_PARAMS * sizeof(char *)), *parsed;
	int i = 0;

	if (NULL == command) {
		perror("malloc:");
		return NULL;
	}
	/* separate the input. */
	parsed = strtok(input, sep);
	while(parsed != NULL) {
		command[i++] = parsed;
		parsed = strtok(NULL, sep);
	}
	command[i] = NULL;
	return command;
}

int main(void)
{
	char *input = NULL;
	char **command = NULL;
	pid_t pid;
	struct sigaction s;

	s.sa_handler = sigint_hndlr;
	sigemptyset(&s.sa_mask);
	s.sa_flags = SA_RESTART;
	sigaction(SIGINT, &s, NULL);

	while (1) {
		if (sigsetjmp(env, 1) == JUMP_CODE) {
			printf("\n");
			continue;
		}

		jump_active = 1;

		input = readline("unixsh> ");
		if (0 == *input) {
			/* simply enter. */
			free(input);
			continue;
		} else if (NULL == input) {
			/* Ctrl + D */
			printf("\n");
			exit(EXIT_SUCCESS);
		}

		command = get_input(input);
		if (NULL == command) {
			fprintf(stderr, "comand parsing failed.");
			free(input);
			continue;
		}
		if (!strcmp("cd", command[0])) {
			if (cd(command[1]) < 0)
				perror("cd:");
			free(input);
			free(command);
			continue;
		}
		/* command parsing successful. */
		pid = fork();
		if (pid < 0) {
			perror("fork:");
			free(command);
			return (EXIT_FAILURE);
		} else if (pid == 0) {
			/* child process. */
			struct sigaction s_child;
			s_child.sa_handler = sigint_hndlr;
			sigemptyset(&s_child.sa_mask);
			s_child.sa_flags = SA_RESTART;
			sigaction(SIGINT, &s_child, NULL);

			execvp(command[0], command);
			perror("exec:");
			exit(EXIT_FAILURE);
		} else {
			/* parent process. */
			waitpid(pid, NULL, WUNTRACED);
		}
		if (!input)
			free(input);
		if (!command)
			free(command);
	}
	return (EXIT_SUCCESS);
}

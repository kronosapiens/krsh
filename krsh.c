/* Krsh Shell */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define min(A, B) (((A) < (B)) ? (A) : (B))

int CMDSIZE = 16384;
int CWDSIZE = 1024;
int ARGSIZE = 64;
int HISTSIZE = 100;

int tokenize_cmd(char *cmd, char *tokens[]);
void handle_cd(char *tokens[], int ntokens);
void handle_exec(char *tokens[], int ntokens);
int handle_hist(char *tokens[], int ntokens, char *history[], int nhistory);
int add_hist(char *cmd, char *history[], int nhistory);
void free_tkn(char *tokens[], int ntokens);
int num_pipes(char *tokens[], int ntokens);
int get_ppos(char *tokens[], int ntokens);
int handle(char *tokens[], int ntokens);

int main(int argc, char *argv[])
{
	char cmd[CMDSIZE];

	int ntokens = 0;
	char *tokens[ARGSIZE];

	int nhistory = 0;
	char *history[HISTSIZE];

	int on = 1;

	while (on) {
		free_tkn(tokens, ntokens);

		/* Get input */
		printf("$");
		fgets(cmd, CMDSIZE, stdin);
		cmd[strcspn(cmd, "\n")] = '\0'; /* Remove newline */

		ntokens = tokenize_cmd(cmd, tokens);
		if (!ntokens)
			continue;

		if (!strcmp(tokens[0], "history") && !get_ppos(tokens, ntokens)) {
			nhistory = handle_hist(tokens, ntokens, history, nhistory);
			continue;
		} else {
			nhistory = add_hist(cmd, history, nhistory);
		}

		on = handle(tokens, ntokens);
	}
	exit(0);
}


void free_tkn(char *tokens[], int ntokens)
{
	int i;

	for (i = 0; i < ntokens; i++)
		free(tokens[i]);
}


int tokenize_cmd(char *cmd, char *tokens[])
{
	int ntokens = 0;
	char *p, *q, *token;

	if (!strlen(cmd))
		return ntokens;

	p = (char *) malloc(strlen(cmd));
	if (p == NULL) {
		printf("error: %s\n", strerror(errno));
		return 0;
	}

	strcpy(p, cmd);
	token = strtok(p, " "); /* Maybe strtok_r? */
	while (token) {
		q = (char *) malloc(strlen(token));
		if (q == NULL) {
			printf("error: %s\n", strerror(errno));
			return ntokens;
		}
		strcpy(q, token);
		tokens[ntokens++] = q;
		token = strtok(NULL, " ");
	}
	return ntokens;
}


int handle(char *tokens[], int ntokens)
{
	if (!strcmp(tokens[0], "exit")) {
		free_tkn(tokens, ntokens);
		return 0;

	} else if (!strcmp(tokens[0], "cd")) {
		handle_cd(tokens, ntokens);
		return 1;

	} else {
		handle_exec(tokens, ntokens);
		return 1;
	}
}


void handle_cd(char *tokens[], int ntokens)
{
	char cwd[CWDSIZE];
	char *change;

	if (ntokens == 1) {
		printf("error: must pass directory to change\n");
	} else if (tokens[1][0] == '/') { /* Absolute path */
		chdir(tokens[1]);
	} else {
		getcwd(cwd, CWDSIZE);
		change = strtok(tokens[1], "/");
		while (change) {
			if (!strcmp(change, ".")) {
				; /* Examine next change */
			} else if (!strcmp(change, "..")) {
				/* Remove last part of path */
				*strrchr(cwd, '/') = '\0';
				chdir(cwd);
			} else {
				/* Append to end of path */
				strcat(cwd, "/");
				strcat(cwd, change);
				chdir(cwd);
			}
			change = strtok(NULL, "/");
		}
	}
}

/* This implementation could be rethought */
int handle_hist(char *tokens[], int ntokens, char *history[], int nhistory)
{
	int i;
	long int j;
	char *p;

	if (ntokens == 2) {
		if (!strcmp(tokens[1], "-c"))
			return 0;

		j = strtol(tokens[1], &p, 0);
		if (p != tokens[1]) { /* Valid int passed */
			if (j >= min(nhistory, HISTSIZE)) {
				printf("error: invalid index %ld\n", j);
				return nhistory;
			}

			if (nhistory >= 100)
				j = (nhistory + j) % HISTSIZE;

			ntokens = tokenize_cmd(history[j], tokens);
			handle(tokens, ntokens);
		} else
			printf("error: invalid argument to history\n");

	} else {
		for (i = 0; i < min(nhistory, HISTSIZE); i++) {
			if (nhistory < 100)
				printf("%d %s\n", i, history[i]);
			else
				printf("%d %s\n", i, history[(nhistory + i) % HISTSIZE]);
		}
	}
	return nhistory;
}


int add_hist(char *cmd, char *history[], int nhistory)
{
	char *p;

	p = (char *) malloc(strlen(cmd));
	if (p == NULL) {
		printf("error: %s\n", strerror(errno));
		return nhistory;
	}
	strcpy(p, cmd);
	history[nhistory % HISTSIZE] = p;
	return ++nhistory;
}


void handle_exec(char *tokens[], int ntokens)
{
	pid_t pid;
	int ret, ppos;

	pid = fork();
	if (pid == 0) { /* Child process */
		ppos = get_ppos(tokens, ntokens);
		while (ppos) {
			int pfds[2];

			if (pipe(pfds) != 0) {
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			pid = fork();
			if (pid == 0) {
				close(pfds[0]);
				dup2(pfds[1], 1); /* Write to pipe */
				close(pfds[1]);
				ntokens = ppos;
				ppos = get_ppos(tokens, ntokens);

			} else if (pid < 0) {
				printf("error: %s\n", strerror(errno));
				exit(1);

			} else {
				close(pfds[1]);
				dup2(pfds[0], 0); /* Read from pipe */
				close(pfds[0]);
				tokens = tokens + (ppos + 1); /* Move pointer past pipe */
				ntokens = ntokens - (ppos + 1); /* Num tokens past pipe */

				tokens[ntokens] = NULL;
				execv(tokens[0], tokens);
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
		}
		tokens[ntokens] = NULL;
		execv(tokens[0], tokens);
		printf("error: %s\n", strerror(errno));
		exit(1);

	} else if (pid < 0) {
		printf("error: %s\n", strerror(errno));
		exit(1);

	} else {
		while (pid != wait(&ret))
			;
	}
}

/* Pipe position */
int get_ppos(char *tokens[], int ntokens)
{
	int ppos = 0;
	int i;

	for (i = 0; i < ntokens; i++)
		if (!strcmp(tokens[i], "|"))
			ppos = i;
	return ppos;
}

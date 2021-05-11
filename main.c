#include "main.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/** The max characters of one cmdline (incl. NULL terminator) */
#define MAX_LINE 8192

/** The max number of pipes that can occur in one cmdline */
#define MAX_PIPE 16

#define BIRD "\xF0\x9F\x90\xA6"

/** Whether or not to show the prompt */
int prompt = 1;

char *cmdname;

void exec_pipe_node(node_t *node, int inpfd);

void exec_command_node(node_t *node, int inpfd);

/** Run a node and obtain an exit status. */
int invoke_node(node_t *node) {
    /* You can modify any part of the body of this function.
       Adding new functions is, of course, also allowed. */

    LOG("Invoke: %s", inspect_node(node));

    switch (node->type) {
    case N_COMMAND:
        /* change directory (cd with no argument is not supported.) */
        if (strcmp(node->argv[0], "cd") == 0) {
            if (node->argv[1] == NULL) {
                return 0; // do nothing
            } else if (chdir(node->argv[1]) == -1) {
                perror("cd");
                return errno;
            } else {
                return 0;
            }
        }

        /* Simple command execution (Task 1) */

        exec_command_node(node, STDIN_FILENO);

        break;

    case N_PIPE: /* foo | bar */
        LOG("node->lhs: %s", inspect_node(node->lhs));
        LOG("node->rhs: %s", inspect_node(node->rhs));

        /* Simple command execution (Tasks 2 and 3) */

        ////////////// TEST 2 //////////////
        //
        // int fd[2];
        // pipe(fd);
        // if (fork() == 0) {
        //     close(fd[0]);
        //     dup2(fd[1], STDOUT_FILENO);
        //     close(fd[1]);
        //     execvp(node->lhs->argv[0], node->lhs->argv);
        // } else if (fork() == 0) {
        //     close(fd[1]);
        //     dup2(fd[0], STDIN_FILENO);
        //     close(fd[0]);
        //     execvp(node->rhs->argv[0], node->rhs->argv);
        //     close(STDIN_FILENO);
        // } else {
        //     close(fd[0]);
        //     close(fd[1]);
        //     wait(NULL);
        //     wait(NULL);
        // }

        //////////// 2 pipes case ///////////
        // 
        // int fd1[2];
        // pipe(fd1);
        // if (fork() == 0) {
        //     if (STDIN_FILENO != STDIN_FILENO){
        //         dup2(STDIN_FILENO, STDIN_FILENO);
        //         close(STDIN_FILENO);
        //     }
        //     dup2(fd1[1], STDOUT_FILENO);
        //     close(fd1[0]);
        //     close(fd1[1]);
        //     execvp(node->lhs->argv[0], node->lhs->argv);
        //     close(STDIN_FILENO);
        // } else {
        //     close(fd1[1]);
        //     int fd2[2];
        //     pipe(fd2);
        //     if (fork() == 0) {
        //         if (fd1[0] != STDIN_FILENO){
        //             dup2(fd1[0], STDIN_FILENO);
        //             close(fd1[0]);
        //         }
        //         dup2(fd2[1], STDOUT_FILENO);
        //         close(fd2[0]);
        //         close(fd2[1]);
        //         execvp(node->rhs->lhs->argv[0], node->rhs->lhs->argv);
        //         close(STDIN_FILENO);
        //     } else {
        //         close(fd1[0]);
        //         close(fd2[1]);
        //         if (fork() == 0){
        //             if (fd2[0] != STDIN_FILENO){
        //                 dup2(fd2[0], STDIN_FILENO);
        //                 close(fd2[0]);
        //             }
        //             dup2(STDOUT_FILENO, STDOUT_FILENO);
        //             close(fd2[1]);
        //             execvp(node->rhs->rhs->argv[0], node->rhs->rhs->argv);
        //             close(STDIN_FILENO);
        //         } else {
        //             close(fd2[0]);
        //             wait(NULL);
        //             wait(NULL);
        //             wait(NULL);
        //         }
        //     }
        // }

        //////////// using exec_pipe_node func ////////////
        
        exec_pipe_node(node, STDIN_FILENO);

        break;

    case N_REDIRECT_IN:     /* foo < bar */
    case N_REDIRECT_OUT:    /* foo > bar */
    case N_REDIRECT_APPEND: /* foo >> bar */
        LOG("node->filename: %s", node->filename);

        /* Redirection (Task 4) */

        break;

    case N_SEQUENCE: /* foo ; bar */
        LOG("node->lhs: %s", inspect_node(node->lhs));
        LOG("node->rhs: %s", inspect_node(node->rhs));

        /* Sequential execution (Task A) */

        break;

    case N_AND: /* foo && bar */
    case N_OR:  /* foo || bar */
        LOG("node->lhs: %s", inspect_node(node->lhs));
        LOG("node->rhs: %s", inspect_node(node->rhs));

        /* Branching (Task B) */

        break;

    case N_SUBSHELL: /* ( foo... ) */
        LOG("node->lhs: %s", inspect_node(node->lhs));

        /* Subshell execution (Task C) */

        break;

    default:
        assert(false);
    }
    return 0;
}

void exec_command_node(node_t *node, int inpfd) {
    if (fork() == 0) { // Child process
        if (inpfd != STDIN_FILENO) {
            dup2(inpfd, STDIN_FILENO);
            close(inpfd);
        }
        execvp(node->argv[0], node->argv);
        close(STDIN_FILENO);
        exit(0);
    } else { // Parent process
        if (inpfd != STDIN_FILENO)
            close(inpfd);
        wait(NULL);
    }
}

void exec_pipe_node(node_t *node, int inpfd) {
    if (node->type == N_COMMAND) { // execute last command (pipe to next node is not needed)
        exec_command_node(node, inpfd);
        return;
    }
    if (node->type != N_PIPE) { // Call non-pipe cammand
        invoke_node(node);
        return;
    }
    int fd[2];
    pipe(fd);
    if (fork() == 0) { // child process
        close(fd[0]);
        if (inpfd != STDIN_FILENO){
            dup2(inpfd, STDIN_FILENO);
            close(inpfd);
        }
        if (fd[1] != STDOUT_FILENO){
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
        }
        execvp(node->lhs->argv[0], node->lhs->argv);
        close(STDIN_FILENO);
        exit(0);
    } else { // parent process
        close(fd[1]);
        if (inpfd != STDIN_FILENO)
            close(inpfd);
        exec_pipe_node(node->rhs, fd[0]);
        close(fd[0]);
        wait(NULL);
    }
}

void parse_options(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "qp")) != -1) {
        switch (opt) {
        case 'q': /* -q: quiet */
            l_set_quiet(1);
            break;
        case 'p': /* -p: no-prompt */
            prompt = 0;
            break;
        case '?':
        default:
            fprintf(stderr, "Usage: %s [-q] [-p] [cmdline ...]\n", cmdname);
            exit(EXIT_FAILURE);
        }
    }
}

int invoke_line(char *line) {
    LOG("Input line='%s'", line);
    node_t *node = yacc_parse(line);
    if (node == NULL) {
        LOG("Obtained empty line: ignored");
        return 0;
    }
    if (!l_get_quiet()) {
        dump_node(node, stdout);
    }
    int exit_status = invoke_node(node);
    free_node(node);
    return exit_status;
}

int main(int argc, char **argv) {
    cmdname = argv[0];
    parse_options(argc, argv);
    if (optind < argc) {
        /* Execute each cmdline in the arguments if exists */
        int exit_status;
        for (int i = optind; i < argc; i++) {
            exit_status = invoke_line(argv[i]);
        }
        return exit_status;
    }

    for (int history_id = 1;; history_id++) {
        char line[MAX_LINE];
        if (prompt) {
            // printf("ttsh[%d]> ", history_id);
            printf("ttsh[%d] %s ", history_id, BIRD);
        }
        /* Read one line */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            /* EOF: Ctrl-D (^D) */
            return EXIT_SUCCESS;
        }
        /* Erase line breaks */
        char *p = strchr(line, '\n');
        if (p) {
            *p = '\0';
        }
        invoke_line(line);
    }
}

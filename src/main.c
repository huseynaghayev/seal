#include <stdio.h>
#include <sys/stat.h> /* S_ISDIR, S_ISREG */
#include "sealconf.h"
#if USE_GNU_READL
    #include <readline/readline.h>
    #include <readline/history.h>
#endif

#include "state.h"
#include "vm.h"

#define STREAM_SIZE (1024 * 1024 * 2)
static char STREAM[STREAM_SIZE];
#define SRC_SIZE 512

#define dump_cache(l) do { \
    for (int i = 0; i < (l)->cachedtk_len; i++) { \
        printf("dumped: %s\n", tkname((l)->cachedtks[i].type)); \
    } \
} while (0)

#define clear() (printf("\033[2J\033[H"))

int main(int argc, char **argv)
{
    seal_state *S;
    if (argc > 1) {
        FILE *fp = fopen(argv[1], "r"); 
        struct stat path_stat;
        if (!fp) {
            fprintf(stderr, "seal: cannot open %s: No such file or directory\n", argv[1]);
            return 1;
        } else if (stat(argv[1], &path_stat),
                   (S_ISDIR(path_stat.st_mode))) {
            fprintf(stderr, "seal: cannot read %s: Is a directory\n", argv[1]);
            fclose(fp);
            return 1;
        } else if (!S_ISREG(path_stat.st_mode)) {
            fprintf(stderr, "seal: cannot read %s: Is unusual type\n", argv[1]);
            fclose(fp);
            return 1;
        }
        int read = fread(STREAM, 1, STREAM_SIZE - 1, fp);
        //printf("%d\n", read);
        STREAM[read] = '\0';
        fclose(fp);
        S = seal_state_new();
        S->file_name = argv[1];
        int status = seal_dostring(S, STREAM);
        if (status) {
            fprintf(stderr, "%s\n", S->errmsg);
        }
        seal_state_free(S);
        return status;
    }

    S = seal_state_new();
#if USE_GNU_READL
    char *input;
#else
    char input[SRC_SIZE];
#endif

    setjmp(S->fail_point);

repl:
#if USE_GNU_READL
    if ((input = readline("> ")) != NULL)
        if (*input)
            add_history(input);
#else
    printf("> ");
    if (fgets(input, SRC_SIZE, stdin) == NULL) {
        putchar('\n');
        goto end;
    }
#endif
    input[strlen(input) - 1] = '\0';
    if (strcmp(input, "clear") == 0) {
        clear();
    } else if (strcmp(input, "exit") == 0) {
        goto end;
    } else if (strcmp(input, "stack") == 0) {
        print_stack(S);
    } else if (strcmp(input, "G") == 0) {
        printf("Globals: cap: %d, size %d\n", S->globals->cap, S->globals->len);
    } else {
        if (seal_dostring(S, input)) {
            fprintf(stderr, "%s\n", S->errmsg);
        }
    }

#if USE_GNU_READL
    free(input);
#endif
    goto repl;

end:
    seal_state_free(S);
    return 0;
}

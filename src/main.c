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
        if (strcmp(argv[1], "--version") == 0) {
            printf("Seal %s by Huseyn Aghayev (c) 2024-2026\nhttps://seallang.org\n", SEAL_VERSION);
            return 0;
        }
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
            fprintf(stderr, "%s\n", S->stktrc);
        }
        seal_state_free(S);
        return status;
    }

    S = seal_state_new();
    S->repl_mode = true;
#if USE_GNU_READL
    char *input;
#else
    char input[SRC_SIZE];
#endif

    setjmp(S->fail_point);

repl:
#if USE_GNU_READL
    if ((input = readline("> ")) != NULL) {
        if (*input) {
            add_history(input);
        }
    } else {
        goto end;
    }
#else
    printf("> ");
    if (fgets(input, SRC_SIZE, stdin) == NULL) {
        putchar('\n');
        goto end;
    }
    input[strlen(input) - 1] = '\0';
#endif
    if (strcmp(input, "clear") == 0) {
        clear();
    } else if (strcmp(input, "exit") == 0) {
        goto end;
    }
#if DEBUG
    else if (strcmp(input, "stack") == 0) {
        print_stack(S);
    } else if (strcmp(input, "G") == 0) {
        printf("Globals: cap: %d, size %d\n", S->globals->cap, S->globals->len);
    } else if (strcmp(input, "intern") == 0) {
        struct seal_hashmap *lexs = S->l.lexemes;
        if (lexs) {
            for (int i = 0; i < lexs->cap; i++) {
                if (lexs->entries[i].key) {
                    printf("%s\n", lexs->entries[i].key);
                }
            }
        }
    }
#endif
    else {
        if (seal_dostring(S, input)) {
            fprintf(stderr, "%s\n", S->errmsg);
            fprintf(stderr, "%s\n", S->stktrc);
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

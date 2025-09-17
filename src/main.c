#include <stdio.h>
#include "sealconf.h"
#if USE_GNU_READL
    #include <readline/readline.h>
    #include <readline/history.h>
#endif

#include "lexer.h"

#define SRC_SIZE 512

#define dump_cache(l) do { \
    for (int i = 0; i < (l)->cachedtk_len; i++) { \
        printf("dumped: %s\n", tkname((l)->cachedtks[i].type)); \
    } \
} while (0)

int main(int argc, char **argv)
{
    struct lexer l;
    lexer_init(&l, NULL);
    struct token t;

    if (argc > 1) {
        FILE *fp = fopen(argv[1], "r"); 
        char buf[SRC_SIZE];
        buf[fread(buf, 1, SRC_SIZE, fp) - 1] = '\0';
        fclose(fp);
        l.src = buf;
        if (setjmp(l.fail_point) == 0) {
            while (1) {
                t = lexer_get_token(&l);
                if (t.type < FIRST_WORD_TOKEN) {
                    printf("%c\n", t.type);
                } else {
                    printf("%s ", tkname(t.type));
                    printf("%s\n", t.val);
                }
                if (t.type == TK_EOF)
                    break;
            }
            dump_cache(&l);
        } else {
            return 1;
        }
        return 0;
    }

#if USE_GNU_READL
    char *input;
#else
    char input[SRC_SIZE];
#endif

    setjmp(l.fail_point);
repl:
    lexer_reset(&l);

#if USE_GNU_READL
    if ((input = readline("> ")) != NULL)
        if (*input)
            add_history(input);
#else
    printf("> ");
    if (fgets(input, SRC_SIZE, stdin) == NULL)
        return 0;
#endif

    l.src = input;
    while (1) {
        t = lexer_get_token(&l);
        if (t.type < FIRST_WORD_TOKEN) {
            printf("%c\n", t.type);
        } else {
            printf("%s ", tkname(t.type));
            printf("%s\n", t.val);
        }
        if (t.type == TK_EOF)
            break;
    }
#if USE_GNU_READL
    free(input);
#endif
    goto repl;

    return 0;
}

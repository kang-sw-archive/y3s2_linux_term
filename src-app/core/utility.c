#include "utility.h"
#include <stdlib.h>
#include <string.h>
#include "uEmbedded/uassert.h"

unsigned g_logLv = 0;
typedef struct cmdarg
{
    uint64_t flags;
    char **opts;
    char **vals;
} cmdarg_t;

static inline uint64_t to_flag_alphabet(char p)
{
    int shift = 0;

    if ('0' <= p && p <= '9')
        shift = p - '0';
    else if ('A' <= p && p <= 'Z')
        shift = p - 'A' + ('9' - '0');
    else if ('a' <= p && p <= 'z')
        shift = p - 'a' + (('Z' - 'A') + ('9' - '0'));
    else
        return 0;

    return 1ull << shift;
}

void ParseCommandLineArgs(UCommandLineDescriptor *p, int *argc, char ***argv, bool bRemoveOpts)
{
    uassert(p && argc && argv);
    int *optidx = malloc(*argc * sizeof(int));
    int numopts = 0;
    uint64_t flags;

    // Select all options
    for (size_t i = 0, count = *argc; i < count; i++)
    {
        char *str = (*argv)[i];

        if (str[0] == '-')
        {
            if (str[1] == '-')
            {
                optidx[numopts++] = i;
            }
            else // Given argument is flags ...
            {
                for (size_t k = 1; str[k]; k++)
                    flags |= to_flag_alphabet(str[k]);
            }
        }
    }

    // @todo.

    for (size_t i; i != -1; --i)
    {
        char *opt = (*argv)[optidx[i]];
    }
    free(optidx);
}

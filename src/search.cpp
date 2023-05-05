#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define NDEBUG
#define NVERIFY
#define LOG_NTRACE
#define LOG_NLEAK
//#define CALLGRIND_MODE

#include "../lib/algorithm/algorithm.h"
#include "../lib/logs/log.h"

#include "hash/type.h"
#include "hash/hash.h"
#include "hash/hash_table.h"

#ifdef CALLGRIND_MODE
#include <valgrind/callgrind.h>
#endif

//--------------------------------------------------------------------------------------------------------------------------------
// SETTINGS
//--------------------------------------------------------------------------------------------------------------------------------

const size_t HASH_TABLE_SIZE = 1907;
const char  *HASH_TABLE_TEXT = "data/dictionary.txt";

int        (*HASH_TABLE_KEY_CMP  ) (hash_key fst, hash_key sec) = strcmp;
hash_val   (*HASH_TABLE_HASH_CALC) (hash_key elem)              = hash_crc32;

const int RUN_SEARCH_NUM      = 5000;
const int MAX_DICTIONARY_SIZE = 60000;

//--------------------------------------------------------------------------------------------------------------------------------
// FUNCTION DECLARATION
//--------------------------------------------------------------------------------------------------------------------------------

void run_search();

static double               run_search       (const hash_table *const store, hash_key *lexis_array);
static __always_inline void hash_table_search(const hash_table *const store, hash_key *lexis_array);

static hash_table          *hash_table_init   (                               hash_key *lexis_array);
static char                *hash_table_rebuild(hash_table *const store, const size_t total_key_size);

static hash_key *lexis_array_init(buffer *const dictionary);

//================================================================================================================================
// MAIN
//================================================================================================================================

int main()
{
    run_search();
}

//================================================================================================================================

void run_search()
{
    buffer *dictionary    = buffer_new (HASH_TABLE_TEXT);
    hash_key *lexis_array = lexis_array_init(dictionary);
    hash_table *store     = hash_table_init(lexis_array);
    char *new_key_store   = hash_table_rebuild(store, dictionary->buff_size);

    fprintf(stderr, "search time: %lf ms\n", run_search(store, lexis_array));

    buffer_free(dictionary);
    hash_table_free (store);

    log_free(new_key_store);
    log_free(lexis_array);
}

//--------------------------------------------------------------------------------------------------------------------------------

static double run_search(const hash_table *const store, hash_key *lexis_array)
{
    log_verify(store       != nullptr, NAN);
    log_verify(lexis_array != nullptr, NAN);

    clock_t search_start = clock();

    #ifdef CALLGRIND_MODE
    CALLGRIND_ZERO_STATS;
    #endif

    for (int i = 0; i < RUN_SEARCH_NUM; ++i)
    {
        hash_table_search(store, lexis_array);
    }

    #ifdef CALLGRIND_MODE
    CALLGRIND_DUMP_STATS;
    #endif

    clock_t search_finish = clock();

    return 1000.0 * (search_finish - search_start) / (CLOCKS_PER_SEC * (double) RUN_SEARCH_NUM);
}

//--------------------------------------------------------------------------------------------------------------------------------

static __always_inline void hash_table_search(const hash_table *const store, hash_key *lexis_array)
{
    log_verify(store       != nullptr, (void) 0);
    log_verify(lexis_array != nullptr, (void) 0);

    for (int i = 0; lexis_array[i] != nullptr; ++i)
    {
        hash_table_find(store, lexis_array[i]);
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

static hash_table *hash_table_init(hash_key *lexis_array)
{
    log_verify(lexis_array != nullptr, nullptr);

    hash_table *store = hash_table_new(HASH_TABLE_SIZE, HASH_TABLE_HASH_CALC, HASH_TABLE_KEY_CMP);
    log_verify(store != nullptr, nullptr);

    for (int i = 0; lexis_array[i] != nullptr; ++i)
    {
        hash_table_push_forced(store, lexis_array[i]);
    }

    return store;
}

//--------------------------------------------------------------------------------------------------------------------------------

static char *hash_table_rebuild(hash_table *const store, const size_t total_key_size)
{
    log_assert(store != nullptr);

    char *const new_key_store = (char *) log_calloc(total_key_size, sizeof(char));
    log_assert(new_key_store != nullptr);

    char *next_key_addr = new_key_store;
    for (size_t chain = 0; chain < HASH_TABLE_SIZE; ++chain)
    {
        list_node *dup_fict = store->data[chain].fictional;
        list_node *dup_cur  = dup_fict + dup_fict->next;

        while (dup_cur != dup_fict)
        {
            size_t key_len = strlen((const char *) dup_cur->data) + 1; //null-character

            memcpy(next_key_addr, dup_cur->data, key_len);

            dup_cur->data = next_key_addr;
            dup_cur = dup_fict + dup_cur->next;

            next_key_addr += key_len;
        }
    }

    return new_key_store;
}

//--------------------------------------------------------------------------------------------------------------------------------

static hash_key *lexis_array_init(buffer *const dictionary)
{
    log_verify(dictionary != nullptr, nullptr);

    hash_key *lexis_array = (hash_key *) log_calloc(MAX_DICTIONARY_SIZE, sizeof(char *));
    hash_key  lexis = strtok(dictionary->buff_beg, "\n");

    int array_ind = 0;
    while (lexis != nullptr)
    {
        lexis_array[array_ind] = lexis;
        lexis = strtok(nullptr, "\n");

        array_ind ++;
    }
    lexis_array[array_ind] = nullptr;

    return lexis_array;
}

#ifndef SEAL_TYPES_H
#define SEAL_TYPES_H

#include "sealconf.h"

#define SEAL_NULL        (1 << 0)    /* 00000001 */
#define SEAL_INT         (1 << 1)    /* 00000010 */
#define SEAL_FLOAT       (1 << 2)    /* 00000100 */
#define SEAL_STRING      (1 << 3)    /* 00001000 */
#define SEAL_BOOL        (1 << 4)    /* 00010000 */
#define SEAL_LIST        (1 << 5)    /* 00100000 */
#define SEAL_MAP         (1 << 6)    /* 01000000 */
#define SEAL_FUNC        (1 << 7)    /* 10000000 */
#define SEAL_MOD         (1 << 8)   /* 100000000 */
#define SEAL_PTR         (1 << 9)  /* 1000000000 */
#define SEAL_NUMBER      (SEAL_INT | SEAL_FLOAT)    /* 00000110 */
#define SEAL_ITERABLE    (SEAL_STRING | SEAL_LIST)  /* 00000110 */
#define SEAL_ANY         (SEAL_NULL | SEAL_INT | SEAL_FLOAT | SEAL_STRING | \
                          SEAL_BOOL | SEAL_LIST | SEAL_MAP | SEAL_FUNC | SEAL_MOD | SEAL_PTR)  /* 1111111111 */


typedef int seal_type;
typedef struct svalue svalue_t;
typedef struct hashmap hashmap_t;


struct line_info {
  int line;
  int offset;
};

struct seal_func {
  enum {
    FUNC_BUILTIN,
    FUNC_USERDEF
  } type;
  union {
    struct {
      seal_byte *bytecode;
      struct line_info *linfo;
      int linfo_size;
      svalue_t *const_pool;
      seal_word *label_pool;
      seal_byte  argc;
      seal_byte  local_size;
      hashmap_t *globals;
      const char *file_name;
    } userdef;
    struct {
      svalue_t (*cfunc)(seal_byte argc, svalue_t* argv);
      seal_byte argc;
    } builtin;
  } as;
  const char* name;
  bool is_vararg;
};

struct seal_string {
  const char* val;
  int size;
  int ref_count;
  bool is_static;
};

struct seal_list {
  svalue_t *mems;
  size_t size;
  size_t cap;
  int ref_count;
};

#define LIST_PUSH(s, e) do { \
  struct seal_list *l = AS_LIST(s); \
  if (l->size >= l->cap) { \
    l->mems = SEAL_REALLOC(l->mems, sizeof(svalue_t) * (l->cap *= 2)); \
  } \
  l->mems[l->size++] = e; \
} while (0)

typedef struct shashmap shashmap_t;

struct seal_map {
  shashmap_t *map;
  int ref_count;
};

#define MAP_INSERT(s, k, v) do { \
  shashmap_insert(AS_MAP(s)->map, k, v); \
} while (0)


struct seal_module {
  hashmap_t *globals;
  const char *name;
};

struct seal_pointer {
  void *ptr;
  const char *name;
};

struct svalue {
  seal_type type;
  union {
    seal_int    _int;
    seal_float  _float;
    struct seal_string *string;
    bool        _bool;
    struct seal_func func;
    struct seal_list *list;
    struct seal_map *map;
    struct seal_module *mod;
    struct seal_pointer ptr;
  } as;
};

struct sh_entry {
  unsigned int hash;
  const char* key;
  svalue_t val;
  bool is_tombstone;
};

typedef struct shashmap {
  struct sh_entry* entries;
  size_t cap;
  size_t filled;
} shashmap_t;

static inline unsigned int shash_str(const char* key) {
  unsigned int hash = 0;
  while (*key)
    hash = *key++ + (hash << 6) + (hash << 16) - hash;

  return hash;
}

static inline void shashmap_init(shashmap_t* hashmap, size_t size)
{
  hashmap->cap = size;
  hashmap->filled = 0;
  hashmap->entries = SEAL_CALLOC(size, sizeof(struct sh_entry));
  for (int i = 0; i < size; i++) {
    hashmap->entries[i].key = NULL;
    hashmap->entries[i].is_tombstone = false;
  }
}

static inline struct sh_entry* shashmap_search(shashmap_t* hashmap, const char* key)
{
  unsigned int idx = shash_str(key) % hashmap->cap;
  struct sh_entry* tombstone = NULL;

  struct sh_entry* e;
  unsigned int start_idx = idx;
  do {
    e = &hashmap->entries[idx];
    if (e->key == NULL) {
      if (e->is_tombstone) {
        if (tombstone == NULL)
          tombstone = e;
      } else {
        return tombstone != NULL ? tombstone : e;
      }
    } else if (strcmp(e->key, key) == 0) {
      return e;
    }
    idx = (idx + 1) % hashmap->cap;
  } while (idx != start_idx);

  return NULL;
}

static inline bool shashmap_insert(shashmap_t* hashmap, const char* key, svalue_t val)
{
  if (hashmap->filled >= hashmap->cap) {
    printf("hashmap is full");
    exit(1);
    return false;
  }

  struct sh_entry* searched = shashmap_search(hashmap, key);
  
  struct sh_entry e = {
    shash_str(key),
    key,
    val,
    true
  };

  bool is_new = searched->key == NULL;
  if (is_new)
    hashmap->filled++;

  *searched = e;

  return is_new;
}

#define AS_INT(val)    ((val).as._int)
#define AS_FLOAT(val)  ((val).as._float)
#define AS_NUM(val)    (IS_INT(val) ? AS_INT(val) : AS_FLOAT(val))
#define AS_STRING(_val) ((_val).as.string->val)
#define AS_BOOL(val)   ((val).as._bool)
#define AS_FUNC(val)   ((val).as.func)
#define AS_LIST(val)   ((val).as.list)
#define AS_MAP(val)    ((val).as.map)
#define AS_MOD(val)    ((val).as.mod)
#define AS_PTR(val)    ((val).as.ptr)

#define VAL_TYPE(val)  ((val).type)
#define IS_NULL(val)   (VAL_TYPE(val) == SEAL_NULL)
#define IS_INT(val)    (VAL_TYPE(val) == SEAL_INT)
#define IS_NUM(val)    (VAL_TYPE(val) & SEAL_NUMBER)
#define IS_FLOAT(val)  (VAL_TYPE(val) == SEAL_FLOAT)
#define IS_STRING(val) (VAL_TYPE(val) == SEAL_STRING)
#define IS_BOOL(val)   (VAL_TYPE(val) == SEAL_BOOL)
#define IS_FUNC(val)   (VAL_TYPE(val) == SEAL_FUNC)
#define IS_LIST(val)   (VAL_TYPE(val) == SEAL_LIST)
#define IS_MAP(val)    (VAL_TYPE(val) == SEAL_MAP)
#define IS_MOD(val)    (VAL_TYPE(val) == SEAL_MOD)
#define IS_PTR(val)    (VAL_TYPE(val) == SEAL_PTR)


#define sval(t, mem, val) (svalue_t) { .type = t, .as.mem = val }
#define SEAL_VALUE_NULL   (svalue_t) { .type = SEAL_NULL }
#define SEAL_VALUE_TRUE   sval(SEAL_BOOL, _bool, true)
#define SEAL_VALUE_FALSE  sval(SEAL_BOOL, _bool, false)
#define SEAL_VALUE_INT(val)    sval(SEAL_INT, _int, val)
#define SEAL_VALUE_FLOAT(val)  sval(SEAL_FLOAT, _float, val)
#define SEAL_VALUE_BOOL(val)   sval(SEAL_BOOL, _bool, val)
#define SEAL_VALUE_PTR(val, _name)    sval(SEAL_PTR, ptr, ((struct seal_pointer) {.ptr = val, .name = _name }))


static inline svalue_t SEAL_VALUE_STRING(const char* val)
{
  svalue_t res = {
    .type = SEAL_STRING,
    .as.string = SEAL_CALLOC(1, sizeof(struct seal_string))
  };
  res.as.string->val = val;
  res.as.string->size = strlen(val);
  res.as.string->is_static = false;
  res.as.string->ref_count = 0;
  return res;
}

static inline svalue_t SEAL_VALUE_STRING_STATIC(const char* val)
{
  svalue_t res = SEAL_VALUE_STRING(val);
  res.as.string->is_static = true;
  return res;
}

static inline svalue_t SEAL_VALUE_LIST()
{
  svalue_t res = {
    .type = SEAL_LIST,
    .as.list = SEAL_CALLOC(1, sizeof(struct seal_list))
  };
  AS_LIST(res)->ref_count = 0;
  AS_LIST(res)->cap = 2;
  AS_LIST(res)->size = 0;
  AS_LIST(res)->mems = SEAL_CALLOC(AS_LIST(res)->cap, sizeof(svalue_t));
  return res;
}


static inline svalue_t SEAL_VALUE_MAP()
{
  svalue_t res = {
    .type = SEAL_MAP,
    .as.map = SEAL_CALLOC(1, sizeof(struct seal_map)),
  };
  AS_MAP(res)->map = SEAL_CALLOC(1, sizeof(shashmap_t));
  AS_MAP(res)->ref_count = 0;
  shashmap_init(res.as.map->map, 256);
  return res;
}

static inline const char*
seal_type_name(int type)
{
  switch (type) {
    case SEAL_NULL    : return "null";
    case SEAL_INT     : return "int";
    case SEAL_FLOAT   : return "float";
    case SEAL_STRING  : return "string";
    case SEAL_BOOL    : return "bool";
    case SEAL_LIST    : return "list";
    case SEAL_MAP     : return "map";
    case SEAL_FUNC    : return "function";
    case SEAL_MOD     : return "module";
    case SEAL_PTR     : return "custom";
    case SEAL_NUMBER  : return "number";
    case SEAL_ITERABLE: return "iterable";
    case SEAL_ANY     : return "any";
    default           : return "SEAL TYPE NOT RECOGNIZED";
  }
}

#endif /* SEAL_TYPES_H */

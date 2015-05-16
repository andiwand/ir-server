
#ifndef UTIL_H_
#define UTIL_H_


#include "c_types.h"
#include "memory.h"

#include "debug/debug_on.h"


#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define BETWEEN(x, min, max) (((x) >= (min)) && ((x) <= (max)))

#define STRING(str) ((string_t) {str, sizeof(str) - 1})

#define PRIMITIVE_LENGTH_MAX 8


typedef enum string_match string_match_t;
typedef enum table_visit table_visit_t;

typedef struct string string_t;
typedef struct string_matcher string_matcher_t;
typedef struct buffer buffer_t;
typedef struct stack_buffer stack_buffer_t;
typedef struct ring_buffer ring_buffer_t;
typedef struct node_single node_single_t;
typedef struct list list_t;
typedef struct array_queue array_queue_t;
typedef struct linked_int_entry linked_int_entry_t;
typedef struct linked_hash_entry linked_hash_entry_t;
typedef struct int_table int_table_t;
typedef struct hash_table hash_table_t;
typedef struct stream stream_t;

typedef uint32_t (*hash_cb_t) (void* a);
typedef bool (*equals_cb_t) (void* a, void* b);
typedef bool (*decide_char_cb_t) (char c);
typedef void (*visit_cb_t) (void* element, void* structure, void* reverse);


enum string_match {
	STRING_MATCH_BUSY,
	STRING_MATCH_TRUE,
	STRING_MATCH_FALSE
};

enum table_visit {
	TABLE_VISIT_KEYS,
	TABLE_VISIT_VALUES,
	TABLE_VISIT_KEYS_VALUES,
	TABLE_VISIT_ENTRIES,
};


struct string {
	char* buffer;
	uint16_t length;
};

struct string_matcher {
	uint16_t position;
};

struct buffer {
	uint8_t* start;
	uint16_t length;
};

struct stack_buffer {
	uint8_t* start;
	uint16_t length;
	uint8_t* position;
};

struct ring_buffer {
	uint8_t* buffer;
	uint16_t length;
	uint8_t* head;
	uint16_t size;
};

struct node_single {
	void* data;
	node_single_t* next;
};

struct list {
	node_single_t* head;
	node_single_t* tail;
	uint16_t size;
};

struct array_queue {
	void** buffer;
	uint16_t length;
	void** head;
	uint16_t size;
};

struct linked_int_entry {
	linked_int_entry_t* next;
	uint64_t key;
	void* value;
};

struct linked_hash_entry {
	linked_hash_entry_t* next;
	void* key;
	void* value;
};

struct int_table {
	void* buckets;
	uint16_t length;
	uint16_t size;
};

struct hash_table {
	void* buckets;
	uint16_t length;
	uint16_t size;

	hash_cb_t hash;
	equals_cb_t equals;
};

struct stream {
	stack_buffer_t buffer;
	bool swap_endian;

	uint8_t primitive_buffer[PRIMITIVE_LENGTH_MAX];

	uint16_t position;
	uint16_t length;
};


string_t* string_create(string_t* string, void* buffer, uint16_t length, bool copy);
void string_destroy(string_t* string, bool buffer, bool object);
string_match_t string_starts_with(string_t* string, string_matcher_t* matcher, const char* buffer, uint16_t length);
string_match_t string_match(string_t string, string_matcher_t* matcher, const char* buffer, uint16_t length);

uint32_t default_hash(void* a);
bool default_equals(void* a, void* b);

bool is_whitespace(char c);

char* skip_chars(const char* buffer, uint16_t len, bool (*skip) (char));

stack_buffer_t* stack_buffer_create(stack_buffer_t* stack, uint8_t* buffer, uint16_t length);
void stack_buffer_destroy(stack_buffer_t* stack, bool all);
static inline uint16_t stack_buffer_size(stack_buffer_t* stack) {
	return stack->position - stack->start;
}
static inline uint16_t stack_buffer_left(stack_buffer_t* stack) {
	return stack->length - stack_buffer_size(stack);
}
static inline void stack_buffer_skip(stack_buffer_t* stack, uint16_t n) {
	stack->position += n;
}
static inline void stack_buffer_push(stack_buffer_t* stack, uint8_t b) {
	*(stack->position++) = b;
}
static inline void stack_buffer_pushn(stack_buffer_t* stack, void* buffer, uint16_t length) {
	m_memcpy(stack->position, buffer, length);
	stack_buffer_skip(stack, length);
}
static inline uint8_t stack_buffer_pop(stack_buffer_t* stack) {
	return *(--stack->position);
}
static inline uint8_t stack_buffer_peek(stack_buffer_t* stack) {
	return *(stack->position - 1);
}
static inline void stack_buffer_reset(stack_buffer_t* stack) {
	stack->position = stack->start;
}

bool array_equals(const uint8_t* a, const uint8_t* b, uint16_t len);
uint8_t* array_find_char(const uint8_t* buffer, uint16_t len, char c);
uint8_t* array_find_array(const uint8_t* outer, uint16_t outer_len, const uint8_t* inner, uint16_t inner_len);

ring_buffer_t* ring_create(uint16_t length);
void ring_destroy(ring_buffer_t* ring);
void ring_clear(ring_buffer_t* ring);
void ring_put(ring_buffer_t* ring, uint8_t c);
uint8_t ring_poll(ring_buffer_t* ring);
void ring_remove(ring_buffer_t* ring, uint16_t n);
uint8_t* ring_get(ring_buffer_t* ring, uint16_t i);
bool ring_equals_array(ring_buffer_t* ring, uint16_t start, uint8_t* array, uint16_t length);

list_t* list_create();
void list_destroy(list_t* list);
void list_clear(list_t* list);
void list_add_first(list_t* list, void* data);
void list_add_last(list_t* list, void* data);
bool list_remove_first(list_t* list);
bool list_remove_find(list_t* list, void* data);
void list_visit(list_t* list, visit_cb_t visiter, void* reverse);

array_queue_t* array_queue_create(uint16_t length);
void array_queue_destroy(array_queue_t* queue);
void array_queue_clear(array_queue_t* queue);
void array_queue_put(array_queue_t* queue, void* pointer);
void* array_queue_poll(array_queue_t* queue);
void array_queue_remove(array_queue_t* queue);
void** array_queue_get(array_queue_t* queue, uint16_t i);
void array_queue_visit(array_queue_t* queue, visit_cb_t visiter, void* reverse);

int_table_t* int_table_create(uint16_t buckets);
void int_table_destroy(int_table_t* table);
void int_table_clear(int_table_t* table);
void* int_table_put(int_table_t* table, uint64_t key, void* value);
void* int_table_get(int_table_t* table, uint64_t key);
bool int_table_contains(int_table_t* table, uint64_t key);
void* int_table_remove(int_table_t* table, uint64_t key);

hash_table_t* hash_table_create(uint16_t buckets, hash_cb_t hash, equals_cb_t equals);
void hash_table_destroy(hash_table_t* table);
void hash_table_clear(hash_table_t* table);
void* hash_table_put(hash_table_t* table, void* key, void* value);
void* hash_table_get(hash_table_t* table, void* key);
bool hash_table_contains(hash_table_t* table, void* key);
void* hash_table_remove(hash_table_t* table, void* key);
void hash_table_visit(hash_table_t* table, visit_cb_t visiter, void* reverse, table_visit_t visit);

// TODO: reduce read, write to one?
stream_t* stream_create(stream_t* stream);
void stream_destroy(stream_t* stream, bool all);
static inline uint16_t stream_left(stream_t* stream) {
	return stack_buffer_left(&stream->buffer);
}
static inline void stream_data(stream_t* stream, uint8_t* buffer, uint16_t length) {
	stream->buffer.start = buffer;
	stream->buffer.length = length;
	stack_buffer_reset(&stream->buffer);
}
static inline void stream_reset(stream_t* stream) {
	stream->position = 0;
	stack_buffer_reset(&stream->buffer);
}
bool stream_read_primitive(stream_t* stream, void* primitive, uint8_t length);
bool stream_write_primitive(stream_t* stream, void* primitive, uint8_t length);
bool stream_read(stream_t* stream, uint8_t* buffer, uint16_t length);
bool stream_write(stream_t* stream, uint8_t* buffer, uint16_t length);


#endif /* UTIL_H_ */

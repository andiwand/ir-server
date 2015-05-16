
#include "util.h"

#include "c_types.h"

#include "memory.h"


static bool int_table_find(int_table_t* table, uint64_t key, linked_int_entry_t** entry_ptr, linked_int_entry_t** prev_ptr, uint16_t* bucket_ptr);
static bool hash_table_find(hash_table_t* table, void* key, linked_hash_entry_t** entry_ptr, linked_hash_entry_t** prev_ptr, uint16_t* bucket_ptr);

static inline uint16_t stream_begin(stream_t* stream, uint16_t length);
static inline bool stream_end(stream_t* stream, uint16_t buffer_delta, uint16_t position_delta);
static inline uint8_t* stream_begin_primitive(stream_t* stream, void* primitive);


string_t* string_create(string_t* string, void* buffer, uint16_t length, bool copy) {
	if (string == NULL) string = (string_t*) m_malloc(sizeof(string_t));

	if (buffer == NULL) {
		string->buffer = (char*) m_malloc(length + 1);

		if (copy) {
			m_memcpy(string->buffer, buffer, length);
			string->buffer[length] = '\0';
		}
	} else {
		string->buffer = (char*) buffer;
	}

	string->length = length;

	return string;
}

void string_destroy(string_t* string, bool buffer, bool object) {
	if (buffer) m_free(string->buffer);
	if (object) m_free(string);
}

string_match_t string_starts_with(string_t* string, string_matcher_t* matcher, const char* buffer, uint16_t length) {
	string_match_t result;
	uint16_t i = 0;
	uint16_t j = (matcher == NULL) ? 0 : matcher->position;

	while (true) {
		bool a = (i == length);
		bool b = (j == string->length);
		if (a) {
			result = STRING_MATCH_TRUE;
			break;
		}
		if (b) {
			result = STRING_MATCH_BUSY;
			break;
		}

		if (buffer[i++] != string->buffer[j++]) {
			result = STRING_MATCH_FALSE;
			break;
		}
	}

	if (matcher != NULL) matcher->position = j;
	return result;
}

string_match_t string_match(string_t string, string_matcher_t* matcher, const char* buffer, uint16_t length) {
	string_match_t result;
	uint16_t i = 0;
	uint16_t j = (matcher == NULL) ? 0 : matcher->position;

	while (true) {
		bool a = (i == length);
		bool b = (matcher->position == string.length);
		if (a & b) {
			result = STRING_MATCH_TRUE;
			break;
		}
		if (a) {
			result = STRING_MATCH_FALSE;
			break;
		}
		if (b) {
			result = STRING_MATCH_BUSY;
			break;
		}

		if (buffer[i++] != string.buffer[matcher->position++]) {
			result = STRING_MATCH_FALSE;
			break;
		}
	}

	if (matcher != NULL) matcher->position = j;
	return result;
}

uint32_t default_hash(void* a) {
	return (uint32_t) a;
}

bool default_equals(void* a, void* b) {
	return a == b;
}

bool is_whitespace(char c) {
	switch (c) {
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		return true;
	}
	return false;
}

char* skip_chars(const char* buffer, uint16_t len, decide_char_cb_t skip) {
	const char* result = buffer;
	while (result < buffer + len) {
		if (!skip(*result)) return (char*) result;
		result++;
	}
	return NULL;
}

bool array_equals(const uint8_t* a, const uint8_t* b, uint16_t len) {
	uint16_t i = 0;
	while (i < len) {
		if (a[i] != b[i]) {
			return false;
		}
		i++;
	}
	return true;
}

uint8_t* array_find_char(const uint8_t* buffer, uint16_t len, char c) {
	const uint8_t* result = buffer;
	while (result < buffer + len) {
		if (*result == c) return (uint8_t*) result;
		result++;
	}
	return NULL;
}

uint8_t* array_find_array(const uint8_t* outer, uint16_t outer_len, const uint8_t* inner, uint16_t inner_len) {
	const uint8_t* result = outer;
	while (result + inner_len < outer + outer_len) {
		if (array_equals(result, inner, inner_len)) return (uint8_t*) result;
		result++;
	}
	return NULL;
}

stack_buffer_t* stack_buffer_create(stack_buffer_t* stack, uint8_t* buffer, uint16_t length) {
	if (stack == NULL) stack = (stack_buffer_t*) m_malloc(sizeof(stack_buffer_t));

	if (buffer == NULL) {
		if (length > 0) {
			stack->start = (uint8_t*) m_malloc(length);
		} else {
			stack->start = NULL;
		}
	} else {
		stack->start = buffer;
	}

	stack->position = stack->start;
	stack->length = length;

	return stack;
}

void stack_buffer_destroy(stack_buffer_t* stack, bool all) {
	m_free(stack->start);
	if (all) m_free(stack);
}

ring_buffer_t* ring_create(uint16_t length) {
	ring_buffer_t* result = (ring_buffer_t*) m_malloc(sizeof(ring_buffer_t));
	result->buffer = (uint8_t*) m_malloc(length);
	result->length = length;
	ring_clear(result);
	return result;
}

void ring_destroy(ring_buffer_t* ring) {
	m_free(ring->buffer);
	m_free(ring);
}

void ring_clear(ring_buffer_t* ring) {
	ring->head = ring->buffer;
	ring->size = 0;
}

void ring_put(ring_buffer_t* ring, uint8_t c) {
	*ring_get(ring, ring->size) = c;
	ring->size++;
}

uint8_t ring_poll(ring_buffer_t* ring) {
	uint8_t result = *ring->head;
	ring_remove(ring, 1);
	return result;
}

void ring_remove(ring_buffer_t* ring, uint16_t n) {
	ring->head = ring_get(ring, n);
	ring->size -= n;
}

uint8_t* ring_get(ring_buffer_t* ring, uint16_t i) {
	return ring->buffer + (ring->head - ring->buffer + i) % ring->length;
}

bool ring_equals_array(ring_buffer_t* ring, uint16_t start, uint8_t* array, uint16_t length) {
	uint16_t i = 0;
	while (i < length) {
		if (*ring_get(ring, start + i) != array[i]) return false;
		i++;
	}
	return true;
}

list_t* list_create() {
	list_t* result = (list_t*) m_malloc(sizeof(list_t));
	m_memset(result, 0, sizeof(list_t));
	return result;
}

void list_destroy(list_t* list) {
	list_clear(list);
	m_free(list);
}

void list_clear(list_t* list) {
	node_single_t* node = list->head;
	while (node != NULL) {
		node_single_t* next = node->next;
		m_free(node);
		node = next;
	}
	list->size = 0;
}

void list_add_first(list_t* list, void* data) {
	node_single_t* node = (node_single_t*) m_malloc(sizeof(node_single_t));
	node->next = list->head;
	node->data = data;
	list->head = node;
	if (list->size == 0) list->tail = node;
	list->size++;
}

void list_add_last(list_t* list, void* data) {
	node_single_t* node = (node_single_t*) m_malloc(sizeof(node_single_t));
	node->next = NULL;
	node->data = data;
	if (list->size == 0) list->head = node;
	else list->tail->next = node;
	list->tail = node;
	list->size++;
}

bool list_remove_first(list_t* list) {
	if (list->size <= 0) return false;

	node_single_t* node = list->head;
	list->head = list->head->next;
	m_free(node);
	list->size--;

	if (list->size == 0) list->tail = NULL;

	return true;
}

bool list_remove_find(list_t* list, void* data) {
	if (list->size <= 0) return false;

	if (list->head->data == data) return list_remove_first(list);

	node_single_t* front = list->head;
	node_single_t* node = front->next;
	while (node != NULL) {
		if (node->data == data) {
			front->next = node->next;
			if (node == list->tail) list->tail = front;
			m_free(node);
			list->size--;
			return true;
		}
		front = node;
		node = node->next;
	}

	return false;
}

void list_visit(list_t* list, visit_cb_t visiter, void* reverse) {
	node_single_t* node = list->head;
	while (node != NULL) {
		visiter(node->data, list, reverse);
		node = node->next;
	}
}

array_queue_t* array_queue_create(uint16_t length) {
	array_queue_t* result = (array_queue_t*) m_malloc(sizeof(array_queue_t));

	result->buffer = (void**) m_malloc(sizeof(void*) * length);
	result->length = length;
	result->head = result->buffer;
	result->size = 0;

	return result;
}

void array_queue_destroy(array_queue_t* queue) {
	m_free(queue->buffer);
	m_free(queue);
}

void array_queue_clear(array_queue_t* queue) {
	queue->head = queue->buffer;
	queue->size = 0;
}

void array_queue_put(array_queue_t* queue, void* pointer) {
	*array_queue_get(queue, queue->size) = pointer;
	queue->size++;
}

void* array_queue_poll(array_queue_t* queue) {
	void* result = queue->head;
	array_queue_remove(queue);
	return result;
}

void array_queue_remove(array_queue_t* queue) {
	queue->head = array_queue_get(queue, 1);
	queue->size--;
}

void** array_queue_get(array_queue_t* queue, uint16_t i) {
	return queue->buffer + (queue->head - queue->buffer + i) % queue->length;
}

void array_queue_visit(array_queue_t* queue, visit_cb_t visiter, void* reverse) {
	uint16_t i = 0;
	while (i < queue->size) {
		visiter(*array_queue_get(queue, i++), queue, reverse);
	}
}

int_table_t* int_table_create(uint16_t buckets) {
	int_table_t* result = (int_table_t*) m_malloc(sizeof(int_table_t));

	result->buckets = (void*) m_malloc(sizeof(linked_int_entry_t*) * buckets);
	result->length = buckets;
	result->size = 0;

	return result;
}

void int_table_destroy(int_table_t* table) {
	int_table_clear(table);

	m_free(table->buckets);
	m_free(table);
}

void int_table_clear(int_table_t* table) {
	uint16_t i = 0;
	while (i < table->length) {
		linked_int_entry_t* node = ((linked_int_entry_t**) table->buckets)[i];
		while (node != NULL) {
			linked_int_entry_t* tmp = node->next;
			m_free(node);
			node = tmp;
		}
		i++;
	}

	table->size = 0;
}

void* int_table_put(int_table_t* table, uint64_t key, void* value) {
	linked_int_entry_t* entry;
	linked_int_entry_t* prev;
	uint16_t bucket;
	if (int_table_find(table, key, &entry, &prev, &bucket)) {
		void* result = entry->value;
		entry->value = value;
		return result;
	}

	entry = (linked_int_entry_t*) m_malloc(sizeof(linked_int_entry_t));
	entry->key = key;
	entry->value = value;
	entry->next = NULL;

	if (prev == NULL) {
		((linked_int_entry_t**) table->buckets)[bucket] = entry;
	} else {
		prev->next = entry;
	}

	table->size++;
	return NULL;
}

void* int_table_get(int_table_t* table, uint64_t key) {
	linked_int_entry_t* entry;
	if (int_table_find(table, key, &entry, NULL, NULL)) return entry->value;
	return NULL;
}

bool int_table_contains(int_table_t* table, uint64_t key) {
	return int_table_find(table, key, NULL, NULL, NULL);
}

void* int_table_remove(int_table_t* table, uint64_t key) {
	linked_int_entry_t* entry;
	linked_int_entry_t* prev;
	uint16_t bucket;
	if (!int_table_find(table, key, &entry, &prev, &bucket)) return NULL;

	if (prev == NULL) {
		((linked_int_entry_t**) table->buckets)[bucket] = entry->next;
	} else {
		prev->next = entry->next;
	}

	void* result = entry->value;
	m_free(entry);
	table->size--;
	return result;
}

static bool int_table_find(int_table_t* table, uint64_t key, linked_int_entry_t** entry_ptr, linked_int_entry_t** prev_ptr, uint16_t* bucket_ptr) {
	bool result = false;
	// TODO: use hash function?
	uint16_t bucket = key % table->length;
	linked_int_entry_t* entry = ((linked_int_entry_t**) table->buckets)[bucket];
	linked_int_entry_t* prev = NULL;

	while (entry != NULL) {
		// TODO: use equals function?
		if (key == entry->key) {
			result = true;
			break;
		}
		prev = entry;
		entry = entry->next;
	}

	if (bucket_ptr != NULL) *bucket_ptr = bucket;
	if (entry_ptr != NULL) *entry_ptr = entry;
	if (prev_ptr != NULL) *prev_ptr = prev;
	return result;
}

hash_table_t* hash_table_create(uint16_t buckets, hash_cb_t hash, equals_cb_t equals) {
	hash_table_t* result = (hash_table_t*) m_malloc(sizeof(hash_table_t));

	result->buckets = (void*) m_malloc(sizeof(linked_hash_entry_t*) * buckets);
	result->length = buckets;
	result->size = 0;
	result->hash = (hash == NULL) ? default_hash : hash;
	result->equals = (equals == NULL) ? default_equals : equals;

	return result;
}

void hash_table_destroy(hash_table_t* table) {
	hash_table_clear(table);

	m_free(table->buckets);
	m_free(table);
}

void hash_table_clear(hash_table_t* table) {
	uint16_t i = 0;
	while (i < table->length) {
		linked_hash_entry_t* node = ((linked_hash_entry_t**) table->buckets)[i];
		while (node != NULL) {
			linked_hash_entry_t* tmp = node->next;
			m_free(node);
			node = tmp;
		}
		i++;
	}

	table->size = 0;
}

void* hash_table_put(hash_table_t* table, void* key, void* value) {
	linked_hash_entry_t* entry;
	linked_hash_entry_t* prev;
	uint16_t bucket;
	if (hash_table_find(table, key, &entry, &prev, &bucket)) {
		void* result = entry->value;
		entry->value = value;
		return result;
	}

	entry = (linked_hash_entry_t*) m_malloc(sizeof(linked_hash_entry_t));
	entry->key = key;
	entry->value = value;
	entry->next = NULL;

	if (prev == NULL) {
		((linked_hash_entry_t**) table->buckets)[bucket] = entry;
	} else {
		prev->next = entry;
	}

	table->size++;
	return NULL;
}

void* hash_table_get(hash_table_t* table, void* key) {
	linked_hash_entry_t* entry;
	if (hash_table_find(table, key, &entry, NULL, NULL)) return entry->value;
	return NULL;
}

bool hash_table_contains(hash_table_t* table, void* key) {
	return hash_table_find(table, key, NULL, NULL, NULL);
}

void* hash_table_remove(hash_table_t* table, void* key) {
	linked_hash_entry_t* entry;
	linked_hash_entry_t* prev;
	uint16_t bucket;
	if (!hash_table_find(table, key, &entry, &prev, &bucket)) return NULL;

	if (prev == NULL) {
		((linked_hash_entry_t**) table->buckets)[bucket] = entry->next;
	} else {
		prev->next = entry->next;
	}

	void* result = entry->value;
	m_free(entry);
	table->size--;
	return result;
}

static bool hash_table_find(hash_table_t* table, void* key, linked_hash_entry_t** entry_ptr, linked_hash_entry_t** prev_ptr, uint16_t* bucket_ptr) {
	bool result = false;
	uint32_t hash = table->hash(key);
	uint16_t bucket = hash % table->length;
	linked_hash_entry_t* entry = ((linked_hash_entry_t**) table->buckets)[bucket];
	linked_hash_entry_t* prev = NULL;

	while (entry != NULL) {
		if (table->equals(key, entry->key)) {
			result = true;
			break;
		}
		prev = entry;
		entry = entry->next;
	}

	if (bucket_ptr != NULL) *bucket_ptr = bucket;
	if (entry_ptr != NULL) *entry_ptr = entry;
	if (prev_ptr != NULL) *prev_ptr = prev;
	return result;
}

void hash_table_visit(hash_table_t* table, visit_cb_t visiter, void* reverse, table_visit_t visit) {
	uint16_t i = 0;
	while (i < table->length) {
		linked_hash_entry_t* entry = ((linked_hash_entry_t**) table->buckets)[i++];
		while (entry != NULL) {
			switch (visit) {
			case TABLE_VISIT_KEYS:
				visiter(entry->key, table, reverse);
				break;
			case TABLE_VISIT_VALUES:
				visiter(entry->value, table, reverse);
				break;
			case TABLE_VISIT_KEYS_VALUES:
				visiter(entry->key, table, reverse);
				visiter(entry->value, table, reverse);
				break;
			case TABLE_VISIT_ENTRIES:
				visiter(entry, table, reverse);
				break;
			default:
				DEBUG("util: unknown table visit");
			}
			entry = entry->next;
		}
	}
}

stream_t* stream_create(stream_t* stream) {
	if (stream == NULL) stream = (stream_t*) m_malloc(sizeof(stream_t));

	stack_buffer_create(&stream->buffer, NULL, 0);
	stream->swap_endian = false;
	memset(stream->primitive_buffer, 0, sizeof(stream->primitive_buffer));
	stream->position = 0;
	stream->length = 0;

	return stream;
}

void stream_destroy(stream_t* stream, bool all) {
	if (all) m_free(stream);
}

static inline uint16_t stream_begin(stream_t* stream, uint16_t length) {
	stream->length = length;

	uint16_t left = stream->length - stream->position;
	uint16_t limit = MIN(left, stream_left(stream));
	return limit;
}

static inline bool stream_end(stream_t* stream, uint16_t buffer_delta, uint16_t position_delta) {
	stream->buffer.position += buffer_delta;
	stream->position += position_delta;

	if (stream->position >= stream->length) {
		stream->position = 0;
		return true;
	}
	return false;
}

static inline uint8_t* stream_begin_primitive(stream_t* stream, void* primitive) {
	if (primitive == NULL) return stream->primitive_buffer;
	return (uint8_t*) primitive;
}

bool stream_read_primitive(stream_t* stream, void* primitive, uint8_t length) {
	uint8_t* p = stream_begin_primitive(stream, primitive);
	uint16_t limit = stream_begin(stream, length);

	if (stream->swap_endian) {
		uint16_t i = 0;
		uint16_t tmp = stream->length - 1 - stream->position;
		while (i < limit) {
			p[tmp - i] = stream->buffer.position[i];
			i++;
		}
	} else {
		m_memcpy(p + stream->position, stream->buffer.position, limit);
	}

	return stream_end(stream, limit, limit);
}

bool stream_write_primitive(stream_t* stream, void* primitive, uint8_t length) {
	uint8_t* p = stream_begin_primitive(stream, primitive);
	uint16_t limit = stream_begin(stream, length);

	if (stream->swap_endian) {
		uint16_t i = 0;
		uint16_t tmp = stream->length - 1 - stream->position;
		while (i < limit) {
			stream->buffer.position[tmp - i] = p[i];
			i++;
		}
	} else {
		m_memcpy(stream->buffer.position, p + stream->position, limit);
	}

	return stream_end(stream, limit, limit);
}

bool stream_read(stream_t* stream, uint8_t* buffer, uint16_t length) {
	uint16_t limit = stream_begin(stream, length);
	m_memcpy(buffer + stream->position, stream->buffer.position, limit);
	return stream_end(stream, limit, limit);
}

bool stream_write(stream_t* stream, uint8_t* buffer, uint16_t length) {
	uint16_t limit = stream_begin(stream, length);
	m_memcpy(stream->buffer.position, buffer + stream->position, limit);
	return stream_end(stream, limit, limit);
}

//https://github.com/vivkin/vjson
#pragma once

class block_allocator
{
private:
	struct block
	{
		size_t size;
		size_t used;

		char *buffer;
		block *next;
	};

	block *m_head;
	size_t m_blocksize;

	block_allocator(const block_allocator &);
	block_allocator &operator=(block_allocator &);

public:
	block_allocator(size_t blocksize);
	~block_allocator();

	// exchange contents with rhs
	void swap(block_allocator &rhs);

	// allocate memory
	void *malloc(size_t size);

	// free all allocated blocks
	void free();
};

enum json_type
{
	JSON_NULL,
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_STRING,
	JSON_INT,
	JSON_FLOAT,
	JSON_BOOL,
};

struct json_value
{
	json_value *parent;
	json_value *next_sibling;
	json_value *first_child;
	json_value *last_child;

	char *name;
	union
	{
		char *string_value;
		int int_value;
		float float_value;
	};

	json_type type;
};

json_value *json_parse(char *source, int source_len, char **error_pos, const char **error_desc, int *error_line, block_allocator *allocator);
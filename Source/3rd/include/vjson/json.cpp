#include <stdafx.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include "json.h"

block_allocator::block_allocator(size_t blocksize): m_head(0), m_blocksize(blocksize)
{
}

block_allocator::~block_allocator()
{
	while (m_head)
	{
		block *temp = m_head->next;
		::free(m_head);
		m_head = temp;
	}
}

void block_allocator::swap(block_allocator &rhs)
{
	std::swap(m_blocksize, rhs.m_blocksize);
	std::swap(m_head, rhs.m_head);
}

void *block_allocator::malloc(size_t size)
{
	if ((m_head && m_head->used + size > m_head->size) || !m_head)
	{
		// calc needed size for allocation
		size_t alloc_size = std::max(sizeof(block) + size, m_blocksize);

		// create new block
		char *buffer = (char *)::malloc(alloc_size);
		block *b = reinterpret_cast<block *>(buffer);
		b->size = alloc_size;
		b->used = sizeof(block);
		b->buffer = buffer;
		b->next = m_head;
		m_head = b;
	}

	void *ptr = m_head->buffer + m_head->used;
	m_head->used += size;
	return ptr;
}

void block_allocator::free()
{
	block_allocator(0).swap(*this);
}

// true if character represent a digit
#define IS_DIGIT(c) (c >= '0' && c <= '9')

// convert string to integer
static char *atoi(char *first, char *last, int *out)
{
	int sign = 1;
	if (first != last)
	{
		if (*first == '-')
		{
			sign = -1;
			++first;
		}
		else if (*first == '+')
		{
			++first;
		}
	}

	int result = 0;
	for (; first != last && IS_DIGIT(*first); ++first)
	{
		result = 10 * result + (*first - '0');
	}
	*out = result * sign;

	return first;
}

// convert hexadecimal string to unsigned integer
static char *hatoui(char *first, char *last, unsigned int *out)
{
	unsigned int result = 0;
	for (; first != last; ++first)
	{
		int digit;
		if (IS_DIGIT(*first))
		{
			digit = *first - '0';
		}
		else if (*first >= 'a' && *first <= 'f')
		{
			digit = *first - 'a' + 10;
		}
		else if (*first >= 'A' && *first <= 'F')
		{
			digit = *first - 'A' + 10;
		}
		else
		{
			break;
		}
		result = 16 * result + (unsigned int)digit;
	}
	*out = result;

	return first;
}

// convert string to floating point
static char *atof(char *first, char *last, float *out)
{
	// sign
	float sign = 1;
	if (first != last)
	{
		if (*first == '-')
		{
			sign = -1;
			++first;
		}
		else if (*first == '+')
		{
			++first;
		}
	}

	// integer part
	float result = 0;
	for (; first != last && IS_DIGIT(*first); ++first)
	{
		result = 10 * result + (float)(*first - '0');
	}

	// fraction part
	if (first != last && *first == '.')
	{
		++first;

		float inv_base = 0.1f;
		for (; first != last && IS_DIGIT(*first); ++first)
		{
			result += (float)(*first - '0') * inv_base;
			inv_base *= 0.1f;
		}
	}

	// result w\o exponent
	result *= sign;

	// exponent
	bool exponent_negative = false;
	int exponent = 0;
	if (first != last && (*first == 'e' || *first == 'E'))
	{
		++first;

		if (*first == '-')
		{
			exponent_negative = true;
			++first;
		}
		else if (*first == '+')
		{
			++first;
		}

		for (; first != last && IS_DIGIT(*first); ++first)
		{
			exponent = 10 * exponent + (*first - '0');
		}
	}

	if (exponent)
	{
		float power_of_ten = 10;
		for (; exponent > 1; exponent--)
		{
			power_of_ten *= 10;
		}

		if (exponent_negative)
		{
			result /= power_of_ten;
		}
		else
		{
			result *= power_of_ten;
		}
	}

	*out = result;

	return first;
}

static inline json_value *json_alloc(block_allocator *allocator)
{
	json_value *value = (json_value *)allocator->malloc(sizeof(json_value));
	memset(value, 0, sizeof(json_value));
	return value;
}

static inline void json_append(json_value *lhs, json_value *rhs)
{
	rhs->parent = lhs;
	if (lhs->last_child)
	{
		lhs->last_child = lhs->last_child->next_sibling = rhs;
	}
	else
	{
		lhs->first_child = lhs->last_child = rhs;
	}
}

#define JSON_ERROR(it, desc)\
	*JSON_ERROR_pos = it;\
	*JSON_ERROR_desc = desc;\
	*JSON_ERROR_line = 1 - escaped_newlines;\
	for (char *c = it; c != source; --c)\
		if (*c == '\n') ++*JSON_ERROR_line;\
	return 0

#define CHECK_TOP() if (!top) {JSON_ERROR(it, "Unexpected character");}

json_value *json_parse(char *source, int source_len, char **JSON_ERROR_pos, const char **JSON_ERROR_desc, int *JSON_ERROR_line, block_allocator *allocator)
{
	json_value *root = 0;
	json_value *top = 0;

	char *name = 0;
	char *it = source;

	int escaped_newlines = 0;

	while (source_len > 0)
	{
		// skip white space
		while (*it == '\x20' || *it == '\x9' || *it == '\xD' || *it == '\xA')
		{
			++it;
			--source_len;
		}

		switch (*it)
		{
		case '\0':
			break;
		case '{':
		case '[':
			{
				// create new value
				json_value *object = json_alloc(allocator);

				// name
				object->name = name;
				name = 0;

				// type
				object->type = (*it == '{') ? JSON_OBJECT : JSON_ARRAY;

				// skip open character
				++it;
				--source_len;

				// set top and root
				if (top)
				{
					json_append(top, object);
				}
				else if (!root)
				{
					root = object;
				}
				else
				{
					JSON_ERROR(it, "Second root. Only one root allowed");
				}
				top = object;
			}
			break;

		case '}':
		case ']':
			{
				if (!top || top->type != ((*it == '}') ? JSON_OBJECT : JSON_ARRAY))
				{
					JSON_ERROR(it, "Mismatch closing brace/bracket");
				}

				// skip close character
				++it;
				--source_len;

				// set top
				top = top->parent;
			}
			break;

		case ':':
			if (!top || top->type != JSON_OBJECT)
			{
				JSON_ERROR(it, "Unexpected character");
			}
			++it;
			--source_len;
			break;

		case ',':
			CHECK_TOP();
			++it;
			--source_len;
			break;

		case '"':
			{
				CHECK_TOP();

				// skip '"' character
				++it;
				--source_len;

				char *first = it;
				char *last = it;
				while (source_len > 0)
				{
					if ((unsigned char)*it < '\x20')
					{
						JSON_ERROR(first, "Control characters not allowed in strings");
					}
					else if (*it == '\\')
					{
						switch (it[1])
						{
						case '"':
							*last = '"';
							break;
						case '\\':
							*last = '\\';
							break;
						case '/':
							*last = '/';
							break;
						case 'b':
							*last = '\b';
							break;
						case 'f':
							*last = '\f';
							break;
						case 'n':
							*last = '\n';
							++escaped_newlines;
							break;
						case 'r':
							*last = '\r';
							break;
						case 't':
							*last = '\t';
							break;
						case 'u':
							{
								unsigned int codepoint;
								if (hatoui(it + 2, it + 6, &codepoint) != it + 6)
								{
									JSON_ERROR(it, "Bad unicode codepoint");
								}

								if (codepoint <= 0x7F)
								{
									*last = (char)codepoint;
								}
								else if (codepoint <= 0x7FF)
								{
									*last++ = (char)(0xC0 | (codepoint >> 6));
									*last = (char)(0x80 | (codepoint & 0x3F));
								}
								else if (codepoint <= 0xFFFF)
								{
									*last++ = (char)(0xE0 | (codepoint >> 12));
									*last++ = (char)(0x80 | ((codepoint >> 6) & 0x3F));
									*last = (char)(0x80 | (codepoint & 0x3F));
								}
							}
							it += 4;
							source_len -= 4;
							break;
						default:
							JSON_ERROR(first, "Unrecognized escape sequence");
						}

						++last;
						it += 2;
						source_len -= 2;
					}
					else if (*it == '"')
					{
						*last = 0;
						++it;
						--source_len;
						break;
					}
					else
					{
						*last++ = *it++;
						--source_len;
					}
				}

				if (!name && top->type == JSON_OBJECT)
				{
					// field name in object
					name = first;
				}
				else
				{
					// new string value
					json_value *object = json_alloc(allocator);

					object->name = name;
					name = 0;

					object->type = JSON_STRING;
					object->string_value = first;

					json_append(top, object);
				}
			}
			break;

		case 'n':
		case 't':
		case 'f':
			{
				CHECK_TOP();

				// new null/bool value
				json_value *object = json_alloc(allocator);

				object->name = name;
				name = 0;

				// null
				if (it[0] == 'n' && it[1] == 'u' && it[2] == 'l' && it[3] == 'l')
				{
					object->type = JSON_NULL;
					it += 4;
					source_len -= 4;
				}
				// true
				else if (it[0] == 't' && it[1] == 'r' && it[2] == 'u' && it[3] == 'e')
				{
					object->type = JSON_BOOL;
					object->int_value = 1;
					it += 4;
					source_len -= 4;
				}
				// false
				else if (it[0] == 'f' && it[1] == 'a' && it[2] == 'l' && it[3] == 's' && it[4] == 'e')
				{
					object->type = JSON_BOOL;
					object->int_value = 0;
					it += 5;
					source_len -= 5;
				}
				else
				{
					JSON_ERROR(it, "Unknown identifier");
				}

				json_append(top, object);
			}
			break;

		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				CHECK_TOP();

				// new number value
				json_value *object = json_alloc(allocator);

				object->name = name;
				name = 0;

				object->type = JSON_INT;

				char *first = it;
				while (*it != '\x20' && *it != '\x9' && *it != '\xD' && *it != '\xA' && *it != ',' && *it != ']' && *it != '}')
				{
					if (*it == '.' || *it == 'e' || *it == 'E')
					{
						object->type = JSON_FLOAT;
					}
					++it;
					--source_len;
				}

				if (object->type == JSON_INT && atoi(first, it, &object->int_value) != it)
				{
					JSON_ERROR(first, "Bad integer number");
				}

				if (object->type == JSON_FLOAT && atof(first, it, &object->float_value) != it)
				{
					JSON_ERROR(first, "Bad float number");
				}

				json_append(top, object);
			}
			break;

		default:
			JSON_ERROR(it, "Unexpected character");
		}
	}

	if (top)
	{
		JSON_ERROR(it, "Not all objects/arrays have been properly closed");
	}

	return root;
}
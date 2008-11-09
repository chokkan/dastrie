/*
 *      Static Double-Array Trie (DASTrie)
 *
 * Copyright (c) 2008, Naoaki Okazaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Northwestern University, University of Tokyo,
 *       nor the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id:$ */

/*
This code assumes that elements of a vector are in contiguous memory,
based on the following STL defect report:
http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#69
*/

#ifndef __DASTRIE_H__
#define __DASTRIE_H__

#include <algorithm>
#include <cstring>
#include <map>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <stdint.h>

#define DASTRIE_MAJOR_VERSION   1
#define DASTRIE_MINOR_VERSION   0
#define DASTRIE_COPYRIGHT       "Copyright (c) 2008 Naoaki Okazaki"

namespace dastrie {


/** 
 * \addtogroup dastrie_api DASTrie API
 * @{
 *
 *	The DASTrie API.
 */

/**
 * Global constants.
 */
enum {
    /// Invalid index number for a double array.
    INVALID_INDEX = 0,
    /// Initial index for a double array.
    INITIAL_INDEX = 1,
    /// Number of characters.
    NUMCHARS = 256,
    /// The size of a chunk header.
    CHUNKSIZE = 8,
    /// The size of a "SDAT" chunk.
    SDAT_CHUNKSIZE = 16,
};



/**
 * Attributes and operations for a double array (4 bytes/element).
 */
struct doublearray4_traits
{
    /// A type that represents an element of a base array.
    typedef int32_t base_type;
    /// A type that represents an element of a check array.
    typedef uint8_t check_type;
    /// A type that represents an element of a double array.
    typedef int32_t element_type;

    /// The chunk ID.
    inline static const char *chunk_id()
    {
        static const char *id = "SDA4";
        return id;
    }

    /// The minimum number of BASE values.
    inline static base_type min_base()
    {
        return 1;
    }

    /// The maximum number of BASE values.
    inline static base_type max_base()
    {
        return 0x007FFFFF;
    }

    /// The default value of an element.
    inline static element_type default_value()
    {
        return 0;
    }

    /// Gets the BASE value of an element.
    inline static base_type get_base(const element_type& elem)
    {
        return (elem >> 8);
    }

    /// Gets the CHECK value of an element.
    inline static check_type get_check(const element_type& elem)
    {
        return (check_type)(elem & 0x000000FF);
    }

    /// Sets the BASE value of an element.
    inline static void set_base(element_type& elem, base_type v)
    {
        elem = (elem & 0x000000FF) | (v << 8);
    }

    /// Sets the CHECK value of an element.
    inline static void set_check(element_type& elem, check_type v)
    {
        elem = (elem & 0xFFFFFF00) | (base_type)v;
    }
};

/**
 * Attributes and operations for a double array (5 bytes/element).
 */
struct doublearray5_traits
{
    /// A type that represents an element of a base array.
    typedef int32_t base_type;
    /// A type that represents an element of a check array.
    typedef uint8_t check_type;
    /// A type that represents an element of a double array.
    struct element_type
    {
        // BASE: v[0:4], CHECK: v[4]
        uint8_t v[5];
    };

    /// The chunk ID.
    inline static const char *chunk_id()
    {
        static const char *id = "SDA5";
        return id;
    }

    /// Gets the minimum number of BASE values.
    inline static base_type min_base()
    {
        return 1;
    }

    /// Gets the maximum number of BASE values.
    inline static base_type max_base()
    {
        return 0x7FFFFFFF;
    }

    /// The default value of an element.
    inline static element_type default_value()
    {
        static const element_type def = {0, 0, 0, 0, 0};
        return def;
    }

    /// Gets the BASE value of an element.
    inline static base_type get_base(const element_type& elem)
    {
        base_type b;
        std::memcpy(&b, &elem.v[0], sizeof(b));
        return b;
    }

    /// Gets the CHECK value of an element.
    inline static check_type get_check(const element_type& elem)
    {
        return elem.v[4];
    }

    /// Sets the BASE value of an element.
    inline static void set_base(element_type& elem, base_type v)
    {
        std::memcpy(&elem.v[0], &v, sizeof(v));
    }

    /// Sets the CHECK value of an element.
    inline static void set_check(element_type& elem, check_type v)
    {
        elem.v[4] = v;
    }
};



/**
 * An unextendable array.
 *  @param  value_tmpl  The element type to be stored in the array.
 */
template <class value_tmpl>
class array
{
public:
    /// The type that represents elements of the array.
    typedef value_tmpl value_type;
    /// The type that represents the size of the array.
    typedef size_t size_type;

protected:
    value_type* m_block;
    size_type   m_size;
    bool        m_own;

public:
    /// Constructs an array.
    array()
        : m_block(NULL), m_size(0), m_own(false)
    {
    }

    /// Constructs an array from an existing memory block.
    array(value_type* block, size_type size, bool own = false)
        : m_block(NULL), m_size(0), m_own(false)
    {
        assign(block, size, own);
    }

    /// Constructs an array from another array instance.
    array(const array& rho)
        : m_block(NULL), m_size(0), m_own(false)
    {
        assign(rho.m_block, rho.m_size, rho.m_own);
    }

    /// Destructs an array.
    virtual ~array()
    {
        free();
    }

    /// Assigns the new array to this instance.
    array& operator=(const array& rho)
    {
        assign(rho.m_block, rho.m_size, rho.m_own);
        return *this;
    }

    /// Obtains a read/write access to an element in the array.
    inline value_type& operator[](size_type i)
    {
        return m_block[i];
    }

    /// Obtains a read-only access to an element in the array.
    inline const value_type& operator[](size_type i) const
    {
        return m_block[i];
    }

    /// Checks whether an array is allocated.
    inline operator bool() const
    {
        return (m_block != NULL);
    }

    /// Reports the size of the array.
    inline size_type size() const
    {
        return m_size;
    }

    /// Assigns a new array from an existing memory block.
    inline void assign(value_type* block, size_type size, bool own = false)
    {
        free();

        if (own) {
            // Allocate a memory block and copy the source array to the block.
            m_block = new value_type[size];
            std::memcpy(m_block, block, sizeof(value_type) * size);
        } else {
            // Just store the pointer to the source array.
            m_block = block;
        }
        m_size = size;
        m_own = own;
    }

    /// Destroy the array.
    inline void free()
    {
        // Free the memory block only when it was allocated by this class.
        if (m_own) {
            delete[] m_block;
        }
        m_block = NULL;
        m_size = 0;
        m_own = false;
    }
};



/**
 * A writer class for a tail array.
 */
class otail
{
public:
    /// The type that represents an element of a tail array.
    typedef uint8_t element_type;
    /// The container for the tail array.
    typedef std::vector<element_type> container_type;
    /// The type that represents the size of the tail array.
    typedef container_type::size_type size_type;

protected:
    /// The tail array.
    container_type m_cont;

public:
    /**
     * Constructs an instance.
     */
    otail()
    {
    }

    /**
     * Destructs an instance.
     */
    virtual ~otail()
    {
    }

    /**
     * Obtains a read-only access to the pointer of the tail array.
     *  @return const element_type* The pointer to the tail array.
     */
    inline const element_type* block() const
    {
        return &m_cont[0];
    }

    /**
     * Reports the size of the tail array.
     *  @return size_type   The size, in bytes, of the tail array.
     */
    inline size_type bytes() const
    {
        return sizeof(element_type) * m_cont.size();
    }

    /**
     * Reports the offset position to which a next data is written.
     *  @return size_type   The current position.
     */
    inline size_type tellp() const
    {
        return this->bytes();
    }

    /**
     * Removes all of the contents in the tail array.
     */
    inline void clear()
    {
        m_cont.clear();
    }

    /**
     * Puts a byte stream to the tail array.
     *  @param  data        The pointer to the byte stream.
     *  @param  size        The size, in bytes, of the byte stream.
     *  @return otail&      The reference to this object.
     */
    inline otail& write(const void *data, size_t size)
    {
        size_type offset = this->bytes();
        if (0 < size) {
            m_cont.resize(offset + size);
            std::memcpy(&m_cont[offset], data, size);
        }
        return *this;
    }

    /**
     * Puts a value of a basic type to the tail array.
     *  @param  value_type  The type of the value.
     *  @param  value       The reference to the value.
     *  @return otail&      The reference to this object.
     */
    template <typename value_type>
    inline otail& write(const value_type& value)
    {
        return write(&value, sizeof(value));
    }

    /**
     * Puts a null-terminated string.
     *  @param  str         The pointer to the string.
     *  @param  offset      The offset from which the string is written.
     *  @return otail&      The reference to this object.
     */
    inline otail& write_string(const char *str, size_type offset = 0)
    {
        return write(str + offset, std::strlen(str + offset) + 1);
    }

    /**
     * Puts a C++ string.
     *  @param  str         The string.
     *  @param  offset      The offset from which the string is written.
     *  @return otail&      The reference to the otail object.
     */
    inline otail& write_string(const std::string& str, size_type offset = 0)
    {
        return write(str.c_str() + offset, str.length() - offset + 1);
    }

    inline otail& operator<<(bool v)            { return write(v); }
    inline otail& operator<<(short v)           { return write(v); }
    inline otail& operator<<(unsigned short v)  { return write(v); }
    inline otail& operator<<(int v)             { return write(v); }
    inline otail& operator<<(unsigned int v)    { return write(v); }
    inline otail& operator<<(long v)            { return write(v); }
    inline otail& operator<<(unsigned long v)   { return write(v); }
    inline otail& operator<<(float v)           { return write(v); }
    inline otail& operator<<(double v)          { return write(v); }
    inline otail& operator<<(long double v)     { return write(v); }
    inline otail& operator<<(const char *str)
    {
        return write_string(str);
    }
    inline otail& operator<<(const std::string& str)
    {
        return write_string(str);
    }
};

/**
 * A reader class for a tail array.
 */
class itail
{
public:
    /// The type that represents an element of a tail array.
    typedef uint8_t element_type;
    /// The container for the tail array.
    typedef array<element_type> container_type;
    /// The type that representing the size of the tail array.
    typedef container_type::size_type size_type;

protected:
    /// The tail array.
    container_type m_cont;
    /// The current reading position.
    size_type m_offset;

public:
    /**
     * Constructs an instance.
     */
    itail() : m_offset(0)
    {
    }

    /**
     * Destructs an instance.
     */
    virtual ~itail()
    {
    }

    /**
     * Checks whether a tail array is allocated.
     *  @return bool        \c true if allocated, \c false otherwise.
     */
    inline operator bool() const
    {
        return m_cont;
    }

    /**
     * Initializes the tail array from an existing memory block.
     *  @param  ptr         The pointer to the memory block of the source.
     *  @param  size        The size of the memory block of the source.
     *  @param  own         \c true to copy the content of the source to a
     *                      new memory block managed by this instance.
     */
    void assign(const element_type* ptr, size_type size, bool own = false)
    {
        m_cont.assign(const_cast<element_type*>(ptr), size, own);
    }

    /**
     * Moves the read position in the tail array.
     *  @param  offset      The offset for the new read position.
     */
    inline void seekg(size_type offset)
    {
        if (offset < m_cont.size()) {
            m_offset = offset;
        }
    }

    /**
     * Reports the current read position in the tail array.
     *  @return size_type   The current position in the tail array.
     */
    inline size_type tellg() const
    {
        return m_offset;
    }

    /**
     * Counts the number of letters in the string from the current position.
     *  @return size_type   The number of letters.
     */
    inline size_type strlen() const
    {
        return std::strlen(reinterpret_cast<const char*>(&m_cont[m_offset]));
    }

    /**
     * Exact match for the string from the current position.
     *  @param  str         The pointer to the string to be compared.
     *  @return bool        \c true if the string starting from the current
     *                      position is identical to the give string str;
     *                      \c false otherwise.
     */
    inline bool match_string(const char *str)
    {
        size_type length = std::strlen(str) + 1;
        if (m_offset + length < m_cont.size()) {
            if (std::memcmp(&m_cont[m_offset], str, length) == 0) {
                return true;
            }
        }
        return false;
    }

    /**
     * Prefix match for the string from the current position.
     *  @param  str         The pointer to the string to be compared.
     *  @return bool        \c true if the give string str begins with the
     *                      substring starting from the current position;
     *                      \c false otherwise.
     */
    inline bool match_string_partial(const char *str)
    {
        size_type length = std::strlen(
            reinterpret_cast<const char *>(&m_cont[m_offset]));
        if (m_offset + length + 1 < m_cont.size()) {
            if (std::memcmp(&m_cont[m_offset], str, length) == 0) {
                if (m_cont[m_offset + length] == 0) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * Gets a byte stream to the tail array.
     *  @param[out] data    The pointer to the byte stream to receive.
     *  @param  size        The size to read.
     *  @return itail&      The reference to this object.
     */
    inline itail& read(void *data, size_t size)
    {
        if (m_offset + size <= m_cont.size()) {
            std::memcpy(data, &m_cont[m_offset], size);
            m_offset += size;
        }
        return *this;
    }

    /**
     * Gets a value of a basic type from the tail array.
     *  @param  value_type  The type of the value.
     *  @param[out] value   The reference to the value.
     *  @return itail&      The reference to this object.
     */
    template <typename value_type>
    inline itail& read(value_type& value)
    {
        return read(&value, sizeof(value));
    }

    inline itail& operator>>(bool& v)           { return read(v); }
    inline itail& operator>>(short& v)          { return read(v); }
    inline itail& operator>>(unsigned short& v) { return read(v); }
    inline itail& operator>>(int& v)            { return read(v); }
    inline itail& operator>>(unsigned int& v)   { return read(v); }
    inline itail& operator>>(long& v)           { return read(v); }
    inline itail& operator>>(unsigned long& v)  { return read(v); }
    inline itail& operator>>(float& v)          { return read(v); }
    inline itail& operator>>(double& v)         { return read(v); }
    inline itail& operator>>(long double& v)    { return read(v); }
    inline itail& operator>>(char *& str)
    {
        str = (char*)&m_cont[m_offset];
        m_offset += strlen() + 1;
        return *this;
    }
    inline itail& operator>>(std::string& str)
    {
        str = reinterpret_cast<const char*>(&m_cont[m_offset]);
        m_offset += strlen() + 1;
        return *this;
    }
};



/**
 * Double Array Trie (read-only).
 *
 *  @param  value_tmpl          A type that represents a record value.
 *  @param  doublearray_traits  A class in which various properties of
 *                              double-array elements are described.
 */
template <class value_tmpl, class doublearray_traits = doublearray5_traits>
class trie
{
public:
    /// A type that represents a record value.
    typedef value_tmpl value_type;
    /// A type that represents an element of a double array.
    typedef typename doublearray_traits::element_type element_type;
    /// A type that represents a base value in a double array.
    typedef typename doublearray_traits::base_type base_type;
    /// A type that represents a check value in a double array.
    typedef typename doublearray_traits::check_type check_type;

    /// A type that implements a container of double-array elements.
    typedef array<element_type> doublearray_type;
    /// A type that represents a size.
    typedef typename doublearray_type::size_type size_type;

    /**
     * Exception class.
     */
    class exception : public std::runtime_error
    {
    public:
        /**
         * Constructs an instance.
         *  @param  msg     The error message.
         */
        explicit exception(const std::string& msg)
            : std::runtime_error(msg)
        {
        }
    };

    /**
     * A cursor clsss for prefix match.
     */
    class prefix_cursor
    {
    protected:
        trie* m_trie;

    public:
        /// The query.
        std::string query;
        /// The length of the prefix.
        size_type   length;
        /// The value of the prefix.
        value_type  value;
        /// The cursor.
        size_type   cur;

    public:
        /**
         * Constructs a cursor.
         */
        prefix_cursor()
            : m_trie(NULL), length(0), cur(INITIAL_INDEX)
        {
        }

        /**
         * Constructs a cursor from a trie and query.
         *  @param  t       The pointer to a trie instance.
         *  @param  q       The query string.
         */
        prefix_cursor(trie* t, const std::string& q)
            : m_trie(t), query(q), length(0), cur(INITIAL_INDEX)
        {
        }

        /**
         * Constructs a cursor from another instance.
         *  @param  rho     The reference to a source instance.
         */
        prefix_cursor(const prefix_cursor& rho)
        {
            m_trie = rho.m_trie;
            query = rho.query;
            cur = rho.cur;
            value = rho.value;
        }

        /**
         * Moves the cursor to the next prefix.
         *  @return         \c true if the trie finds a key string that is a
         *                  prefix of the query string; \c false otherwise.
         */
        bool next()
        {
            return (m_trie != NULL && m_trie->next_prefix(*this));
        }
    };

protected:
    char* m_block;
    uint8_t m_table[NUMCHARS];
    doublearray_type m_da;
    itail m_tail;
    size_type m_n;

public:
    /**
     * Constructs an instance.
     */
    trie()
    {
        m_block = NULL;

        // Initialize the character table.
        for (int i = 0;i < NUMCHARS;++i) {
            m_table[i] = i;
        }
    }

    /**
     * Destructs an instance.
     */
    virtual ~trie()
    {
        if (m_block != NULL) {
            delete[] m_block;
            m_block = NULL;
        }
    }

    /**
     * Gets the number of records in the trie.
     *  @param  size_type   The number of records.
     */
    size_type size() const
    {
        return m_n;
    }

    /**
     * Tests if the trie contains a key.
     *  @param  key         The key string.
     *  @return bool        \c true if the trie contains the key;
     *                      \c false otherwise.
     */
    bool in(const char *key)
    {
        return (locate(key) != 0);
    }

    /**
     * Finds a record.
     *  @param  key         The key string.
     *  @param[out] value   The reference to a variable that receives the
     *                      value of the key.
     *  @return bool        \c true if the trie contains the key;
     *                      \c false otherwise.
     */
    bool find(const char *key, value_type& value)
    {
        size_type offset = locate(key);
        if (offset != 0) {
            m_tail.seekg(offset);
            m_tail >> value;
            return true;
        } else {
            return false;
        }
    }

    /**
     * Gets the value for a key.
     *  @param  key         The key string.
     *  @param  def         The default value.
     *  @return value_type  The value if the key exists in the trie,
     *                      the default value (def) otherwise.
     */
    value_type get(const char *key, const value_type& def)
    {
        value_type value;
        if (find(key, value)) {
            return value;
        } else {
            return def;
        }
    }

    /**
     * Constructs a cursor for prefix match.
     *  @param  str             The query string.
     *  @return prefix_cursor   The instance of a cursor.
     */
    prefix_cursor prefix(const char *str)
    {
        return prefix_cursor(this, str);
    }

    /**
     * Assigns a double-array trie from a builder.
     *  @param  da              The vector of double-array elements.
     *  @param  tail            The tail array.
     *  @param  table           The character-mapping table.
     */
    void assign(
        const std::vector<element_type>& da,
        const otail& tail,
        const uint8_t* table
        )
    {
        m_da.assign(const_cast<element_type*>(&da[0]), da.size(), true);
        m_tail.assign(tail.block(), tail.bytes(), true);
        for (int i = 0;i < NUMCHARS;++i) {
            m_table[i] = table[i];
        }
    }

protected:
    size_type locate(const char *key)
    {
        const char *p = key;
        size_type offset = 0;
        size_type cur = INITIAL_INDEX;
        const uint8_t* table = m_table;

        for (;;) {
            // Try to descend to the child node.
            cur = descend(cur, *reinterpret_cast<const uint8_t*>(p));
            if (cur == INVALID_INDEX) {
                return false;
            }

            base_type base = get_base(cur);
            if (base < 0) {
                // The element #cur is a leaf node.
                if (*p != 0) ++p;
                offset = (size_type)-base;
                break;
            }

            if (*p == 0) {
                // The key string couldn't reach a leaf node.
                return false;
            }
            ++p;
        }

        // Seek to the position of the key postfix in the TAIL.
        m_tail.seekg(offset);

        // Check if two key postfixes are identical.
        if (m_tail.match_string(p)) {
            return offset + m_tail.strlen() + 1;
        } else {
            return 0;
        }
    }

    size_type descend(size_type i, const uint8_t c) const
    {
        const uint8_t* table = m_table;

        base_type base = get_base(i);
        if (base <= 0) {
            // The element #i is not a node.
            return INVALID_INDEX;
        }
        check_type check = (check_type)table[c];
        size_type next = base + (size_type)check + 1;
        if (m_da.size() <= next) {
            // Outside of the double array.
            return INVALID_INDEX;
        }
        if (get_check(next) != check) {
            // The backward link does not exist.
            return INVALID_INDEX;
        }

        return next;
    }

    bool next_prefix(prefix_cursor& pfx)
    {
        const char *p = pfx.query.c_str();
        size_type offset = 0;
        const uint8_t* table = m_table;

        if (std::strlen(p) <= pfx.length) {
            return false;
        }

        for (;;) {
            // Try to descend to the child node.
            pfx.cur = descend(pfx.cur, (uint8_t)p[pfx.length]);
            if (pfx.cur == INVALID_INDEX) {
                return false;
            }

            base_type base = get_base(pfx.cur);
            if (base < 0) {
                // The element #(pfx.cur) is a leaf node.
                if (p[pfx.length] != 0) ++pfx.length;
                offset = (size_type)-base;
                break;
            }

            // Try to descend to the child node with '\0'.
            size_type cur = descend(pfx.cur, 0);
            if (cur != INVALID_INDEX) {
                base = get_base(cur);
                if (base != 0) {
                    if (0 <= base) {
                        throw exception("");
                    }
                    m_tail.seekg((size_type)-base);
                    if (m_tail.strlen() != 0) {
                        throw exception("");
                    }
                    ++pfx.length;
                    m_tail.seekg(((size_type)-base) + 1);
                    m_tail >> pfx.value;
                    return true;
                }
            }

            if (p[pfx.length] == 0) {
                // The key string couldn't reach a leaf node.
                return false;
            }
            ++pfx.length;
        }

        // Seek to the position of the key postfix in the TAIL.
        m_tail.seekg(offset);

        // Check if two key postfixes are identical.
        bool match = m_tail.match_string_partial(&p[pfx.length]);
        if (match) {
            size_type postfix_size = m_tail.strlen();
            pfx.length += postfix_size;
            // Skip the key postfix.
            m_tail.seekg(offset + postfix_size + 1);
            // Read the value.
            m_tail >> pfx.value;
        }
        
        return match;
    }

    inline base_type get_base(size_type i) const
    {
        return doublearray_traits::get_base(m_da[i]);
    }

    inline check_type get_check(size_type i) const
    {
        return doublearray_traits::get_check(m_da[i]);
    }

public:
    /**
     * Assigns a double-array trie from a memory image.
     *  @param  block           The pointer to the memory block.
     *  @param  size            The size, in bytes, of the memory block.
     *  @return size_type       If successful, the size, in bytes, of the
     *                          memory block used to read a double-array trie;
     *                          otherwise zero.
     */
    size_type assign(const char *block, size_type size)
    {
        char chunk[4];
        uint32_t value, sdat_size, total_size;
        const uint8_t* p = reinterpret_cast<const uint8_t*>(block);

        // The size of the memory block must not be smaller than SDAT_CHUNKSIZE.
        if (size < SDAT_CHUNKSIZE) {
            return 0;
        }

        // Read the "SDAT" chunk.
        p += read_chunk(p, chunk, total_size);
        if (std::strncmp(chunk, "SDAT", 4) != 0) {
            return 0;
        }

        // Check the size of the "SDAT" chunk.
        p += read_uint32(p, sdat_size);
        if (sdat_size != SDAT_CHUNKSIZE) {
            return 0;
        }

        // Read the number of records in the trie.
        p += read_uint32(p, value);
        m_n = (size_type)value;

        // Loop for child chunks.
        const uint8_t* last = reinterpret_cast<const uint8_t*>(block) + total_size;
        while (p < last) {
            uint32_t size;
            const uint8_t* q = p;
            q += read_chunk(q, chunk, size);
            uint32_t datasize = size - CHUNKSIZE;

            if (strncmp(chunk, "TBLU", 4) == 0) {
                // "TBLU" chunk.
                if (datasize == NUMCHARS) {
                    for (int i = 0;i < NUMCHARS;++i) {
                        m_table[i] = q[i];
                    }
                }

            } else if (strncmp(chunk, doublearray_traits::chunk_id(), 4) == 0) {
                // "SDA4" or "SDA5" chunk.
                m_da.assign((element_type*)q, datasize / sizeof(element_type));

            } else if (strncmp(chunk, "TAIL", 4) == 0) {
                // "TAIL" chunk.
                m_tail.assign(q, datasize);

            }

            p += size;
        }

        // Make sure that arrays are allocated successfully.
        if (!m_da || !m_tail) {
            return 0;
        }

        return total_size;
    }

    /**
     * Read a double-array trie from an input stream.
     *  @param  is              The input stream.
     *  @return size_type       The size of the double-array data.
     */
    size_type read(std::istream& is)
    {
        char chunk[4];
        uint32_t total_size;
        uint8_t data[CHUNKSIZE];
        std::istream::pos_type offset = is.tellg();

        // Read CHUNKSIZE bytes.
        is.read(reinterpret_cast<char*>(data), CHUNKSIZE);
        if (is.fail()) {
            is.seekg(offset, std::ios::beg);
            return 0;
        }

        // Parse the data as a chunk.
        read_chunk(data, chunk, total_size);

        // Make sure that the data is a "SDAT" chunk.
        if (std::strncmp(chunk, "SDAT", 4) != 0) {
            is.seekg(offset, std::ios::beg);
            return 0;
        }

        // Allocate a new memory block and copy the data.
        m_block = new char[total_size];
        std::memcpy(m_block, data, CHUNKSIZE);

        // Read the actual data.
        is.read(m_block + CHUNKSIZE, total_size - CHUNKSIZE);
        if (is.fail()) {
            is.seekg(offset, std::ios::beg);
            return 0;
        }

        // Allocate the trie.
        size_type used_size = assign(m_block, total_size);
        if (used_size != total_size) {
            is.seekg(offset, std::ios::beg);
            return 0;
        }

        return used_size;
    }

protected:
    size_type read_uint32(const uint8_t* block, uint32_t& value)
    {
        return read_data(block, &value, sizeof(value));
    }

    size_type read_data(const uint8_t* block, void *data, size_t size)
    {
        std::memcpy(data, block, size);
        return size;
    }

    size_type read_chunk(const uint8_t* block, char *chunk, uint32_t& size)
    {
        std::memcpy(chunk, block, 4);
        read_uint32(block + 4, size);
        return 8;
    }
};



/**
 * A builder of a double-array trie.
 *
 *  This class builds a double-array trie from records sorted in dictionary
 *  order of keys.
 *
 *  @param  key_tmpl            A type that represents a record key. This type
 *                              must be either \c char* or \c std::string .
 *  @param  value_tmpl          A type that represents a record value.
 *  @param  doublearray_traits  A class in which various properties of
 *                              double-array elements are described.
 */
template <
    class key_tmpl,
    class value_tmpl,
    class doublearray_traits = doublearray5_traits
>
class builder
{
public:
    /// A type that represents a record.
    typedef key_tmpl key_type;
    /// A type that represents a record value.
    typedef value_tmpl value_type;
    /// A type that represents an element of a double array.
    typedef typename doublearray_traits::element_type element_type;
    /// A type that represents a base value in a double array.
    typedef typename doublearray_traits::base_type base_type;
    /// A type that represents a check value in a double array.
    typedef typename doublearray_traits::check_type check_type;
    /// A type that implements a double array.
    typedef std::vector<element_type> doublearray_type;
    /// A type of sizes.
    typedef typename doublearray_type::size_type size_type;

    /**
     * A type that represents a record (a pair of key and value).
     */
    struct record_type
    {
        key_type key;       ///< The key of the record.
        value_type value;   ///< The value of the record.
    };


    /**
     * Exception class.
     */
    class exception : public std::runtime_error
    {
    public:
        /**
         * Constructs an instance.
         *  @param  msg     The error message.
         */
        explicit exception(const std::string& msg)
            : std::runtime_error(msg)
        {
        }
    };

    /**
     * Statistics of the double array trie.
     */
    struct stat_type
    {
        /// The size, in bytes, of the double array.
        size_type   da_size;
        /// The number of elements in the double array.
        size_type   da_num_total;
        /// The number of elements used actually in the double array.
        size_type   da_num_used;
        /// The number of nodes (excluding leaves).
        size_type   da_num_nodes;
        /// The number of leaves.
        size_type   da_num_leaves;
        /// The utilization ratio of the double array.
        double      da_usage;
        /// The size, in bytes, of the tail array.
        size_type   tail_size;
        /// The sum of the number of trials for finding bases.
        size_type   bt_sum_base_trials;
        /// The average number of trials for finding bases.
        double      bt_avg_base_trials;
    };

    /**
     * The type of a progress callback function.
     *  @param  instance    The pointer to a user-defined instance.
     *  @param  i           The number of records that have already been
     *                      stored in the trie.
     *  @param  n           The total number of records to be stored.
     */
    typedef void (*callback_type)(void *instance, size_type i, size_type n);

protected:
    struct dlink_element_type
    {
        size_type prev;
        size_type next;
        dlink_element_type() : prev(0), next(0)
        {
        }
    };
    typedef std::vector<dlink_element_type> dlink_type;

    typedef std::vector<bool> baseusage_type;

    void* m_instance;
    callback_type m_callback;

    size_type m_i;
    size_type m_n;

    doublearray_type m_da;
    otail m_tail;
    uint8_t m_table[NUMCHARS];

    baseusage_type m_used_bases;
    dlink_type m_elink;

    stat_type m_stat;

public:
    /**
     * Constructs a builder.
     */
    builder()
        : m_instance(NULL), m_callback(NULL)
    {
    }

    /**
     * Destructs the builder.
     */
    virtual ~builder()
    {
    }

    /**
     * Sets a progress callback.
     *  @param  instance    The pointer to a user-defined instance.
     *  @param  callback    The callback function.
     */
    void set_callback(void* instance, callback_type callback)
    {
        m_instance = instance;
        m_callback = callback;
    }

    /**
     * Builds a double-array trie from sorted records.
     *  @param  first       The random-access iterator addressing the position
     *                      of the first record.
     *  @param  last        The random-access iterator addressing the position
     *                      one past the final record.
     */
    void build(const record_type* first, const record_type* last)
    {
        clear();

        m_i = 0;
        m_n = (size_t)(last - first);
        build_table(m_table, first, last);

        // Create the initial node.
        da_expand(INITIAL_INDEX+1);
        vlist_expand(INITIAL_INDEX+1);
        set_base(INITIAL_INDEX, 1);
        vlist_use(INITIAL_INDEX);
        set_base(INITIAL_INDEX, arrange(0, first, last));

        // 
        compute_stat();
    }

    /**
     * Initializes the double array.
     */
    void clear()
    {
        // Initialize the character table.
        for (int i = 0;i < NUMCHARS;++i) {
            m_table[i] = i;
        }

        // Initialize the double array.
        m_da.clear();
        da_expand(1);

        // Initialize the tail array.
        m_tail.clear();
        m_tail << (uint8_t)0;

        // Initialize the vacant linked list.
        vlist_init();

        // Initialize the statistics.
        std::memset(&m_stat, 0, sizeof(m_stat));
    }

    /**
     * Obtains a read-only access to the double-array.
     *  @return const doublearray_type& The reference to the double array.
     */
    const doublearray_type& doublearray() const
    {
        return m_da;
    }

    /**
     * Obtains a read-only access to the tail array.
     *  @return const otail&    The reference to the tail array.
     */
    const otail& tail() const
    {
        return m_tail;
    }

    /**
     * Obtains a read-only access to the character table.
     *  @return const uint8_t*  The pointer to the character table.
     */
    const uint8_t* table() const
    {
        return m_table;
    }

    const stat_type& stat() const
    {
        return m_stat;
    }

protected:
    base_type arrange(size_type p, const record_type* first, const record_type* last)
    {
        size_type i;
        const record_type* it;
        const uint8_t* table = m_table;

        // If the given range [first, last) points to a single record, i.e.,
        // (first + 1 == last), store the key postfix and value of the record
        // to the TAIL array; let the current node as a leaf node addressing
        // to the offset from which (*first) are stored in the TAIL array.
        if (first + 1 == last) {
            const record_type& rec = *first;
            size_t offset = m_tail.tellp();
            if ((size_t)doublearray_traits::max_base() < offset) {
                throw exception("The double array has no space to store leaves");
            }
            m_tail.write_string(rec.key, p);
            m_tail << rec.value;

            if (m_callback != NULL) {
                m_callback(m_instance, ++m_i, m_n);
            }
            ++m_stat.da_num_leaves;
            return -(base_type)offset;
        }

        // Build a list of child nodes of the current node, and obtain the
        // range of records that each child node owns. Child nodes consist
        // of a set of characters at records[i].key[p] for i in [begin, end).
        int pc = -1;
        struct child_t {
            uint8_t             c;
            size_type           offset;
            const record_type*  first;
            const record_type*  last;
        } children[NUMCHARS];
        size_type num_children = 0;
        size_type max_offset = 0;
        for (it = first;it != last;++it) {
            int c = (int)(uint8_t)it->key[p];
            if (pc < c) {
                if (0 < num_children) {
                    children[num_children-1].last = it;
                }
                size_type offset = (size_type)table[c] + 1;
                children[num_children].first = it;
                children[num_children].c = (uint8_t)c;
                children[num_children].offset = offset;
                if (max_offset < offset) {
                    max_offset = offset;
                }
                ++num_children;
            } else if (c < pc) {
                throw exception("The records are not sorted in dictionary order of keys");
            }
            pc = c;
        }
        children[num_children-1].last = it;

        // Find the minimum of the base address (base) that can store every
        // child. This step would be very time consuming if we tried base
        // indexes from 1 one by one and tested the vacanies for child nodes.
        // Instead, we try to determine the index number of the first child-
        // node by using a double-linked list of vacant nodes, and calculate
        // back the base address from the index number of the child node.
        size_type base = 0, index = 0;
        for (;;) {
            ++m_stat.bt_sum_base_trials;

            // Obtain the index value of a next vacant node.
            index = vlist_next(index);

            // A base value must be greater than 1.
            if (index < INITIAL_INDEX + children[0].offset) {
                // The index is too small for a base value; try next.
                continue;
            }

            // Calculate back the base value from the index.
            base = index - children[0].offset;

            // A base value must not be used by other places.
            if (base < m_used_bases.size() && m_used_bases[base]) {
                continue;
            }

            // Expand the double array and vacant list if necessary.
            da_expand(base + max_offset + 1);
            vlist_expand(base + max_offset + 1);

            // Check if the base address can store every child in the list.
            for (i = 1;i < num_children;++i) {
                size_type offset = children[i].offset;
                if (da_in_use(base + offset)) {
                    break;
                }
            }

            // Exit the loop if successful.
            if (i == num_children) {
                break;
            }
        }

        // Fail if the double array could not store the child nodes.
        if ((size_type)doublearray_traits::max_base() <= base + max_offset) {
            throw exception("The double array has no space to store child nodes");
        }

        // Register the usage of the base address.
        if (m_used_bases.size() <= base) {
            m_used_bases.resize(base+1, false);
        }
        m_used_bases[base] = true;

        // Reserve the double-array elements for the child nodes by filling
        // BASE = 1 tentatively. This step protects these elements from being
        // used by the descendant nodes; this function recursively builds the
        // double array in depth-first fashion.
        for (i = 0;i < num_children;++i) {
            size_type offset = children[i].offset;
            set_base(base + offset, 1);
            vlist_use(base + offset);
        }

        // Set BASE and CHECK values of each child node.
            for (i = 0;i < num_children;++i) {
            const child_t& child = children[i];
            size_type offset = child.offset;
            if (child.c != 0) {
                // Set the base value of a child node by recursively arranging
                // the descendant nodes.
                set_base(base + offset, arrange(p+1, child.first, child.last));
            } else {
                if (child.first + 1 != child.last) {
                    throw exception("Duplicated keys detected");
                }
                // Force to insert '\0' in the TAIL.
                set_base(base + offset, arrange(p, child.first, child.last));
            }
            set_check(base + offset, (uint8_t)(offset - 1));
        }

        ++m_stat.da_num_nodes;
        return (base_type)base;
    }

    void compute_stat()
    {
        m_stat.da_size = sizeof(m_da[0]) * m_da.size();
        m_stat.da_num_total = m_da.size();
        for (size_type i = 0;i < m_da.size();++i) {
            if (da_in_use(i)) {
                ++m_stat.da_num_used;
            }
        }
        m_stat.da_usage = m_stat.da_num_used / (double)m_stat.da_num_total;
        m_stat.tail_size = m_tail.bytes();
        m_stat.bt_avg_base_trials = m_stat.bt_sum_base_trials / (double)m_stat.da_num_total;
    }

protected:
    inline base_type get_base(size_type i) const
    {
        return doublearray_traits::get_base(m_da[i]);
    }

    inline check_type get_check(size_type i) const
    {
        return doublearray_traits::get_check(m_da[i]);
    }

    inline void set_base(size_type i, base_type v)
    {
        doublearray_traits::set_base(m_da[i], v);
    }

    inline void set_check(size_type i, check_type v)
    {
        doublearray_traits::set_check(m_da[i], v);
    }

    inline bool da_in_use(size_type i) const
    {
        return (i < m_da.size() && get_base(i) != 0);
    }

    inline void da_expand(size_type size)
    {
        if (m_da.size() < size) {
            m_da.resize(size, doublearray_traits::default_value());
        }
    }

    void vlist_init()
    {
        m_elink.resize(1);
        m_elink[0].next = 1;
        m_elink[0].prev = 0;
    }

    inline size_type vlist_next(size_type i)
    {
        return (i < m_elink.size()) ? m_elink[i].next : i+1;
    }

    void vlist_expand(size_type size)
    {
        if (m_elink.size() < size) {
            size_type first = m_elink.size();
            m_elink.resize(size);

            size_type back = m_elink[0].prev;
            for (size_type i = first;i < m_elink.size();++i) {
                m_elink[i].prev = back;
                m_elink[i].next = i+1;
                back = i;
            }
            m_elink[0].prev = m_elink.size()-1;
        }
    }

    void vlist_use(size_type i)
    {
        size_type prev = m_elink[i].prev;
        size_type next = m_elink[i].next;
        if (m_elink.size() <= next) {
            m_elink.resize(next+1);
            m_elink[next].next = next+1;
            m_elink[0].prev = next; // The rightmost vacant node.
        }
        m_elink[prev].next = next;
        m_elink[next].prev = prev;
    }

protected:
    struct unigram_freq
    {
        int c;          ///< Character code.
        double freq;    ///< Frequency.
    };

    static bool comp_freq(const unigram_freq& x, const unigram_freq& y)
    {
        return x.freq > y.freq;
    }

    void build_table(
        uint8_t *table,
        const record_type* first,
        const record_type* last
        )
    {
        unigram_freq st[NUMCHARS];

        // Initialize the frequency table.
        for (int i = 0;i < NUMCHARS;++i) {
            st[i].c = i;
            st[i].freq = 0.;
        }

        // Count the frequency of occurrences of characters.
        for (const record_type* it = first;it != last;++it) {
            for (int i = 0;it->key[i];++i) {
                int c = (int)(uint8_t)it->key[i];
                ++st[c].freq;
            }
            ++st[0].freq;
        }

        // Sort the frequency table.
        std::sort(&st[0], &st[NUMCHARS], comp_freq);

        // 
        for (int i = 0;i < NUMCHARS;++i) {
            table[st[i].c] = (uint8_t)i;
        }
    }


public:
    /**
     * Writes out the double-array trie to an output stream.
     *  @param  os      The output stream.
     */
    void write(std::ostream& os)
    {
        // Calculate the size of each chunk.
        size_type sda_size = CHUNKSIZE + sizeof(m_da[0]) * m_da.size();
        size_type tblu_size = CHUNKSIZE + sizeof(uint8_t) * NUMCHARS;
        size_type tail_size = CHUNKSIZE +  m_tail.bytes();
        size_type total_size = SDAT_CHUNKSIZE + tblu_size + sda_size + tail_size;

        // Write a "SDAT" chunk.
        write_chunk(os, "SDAT", total_size);
        write_uint32(os, (uint32_t)SDAT_CHUNKSIZE);
        write_uint32(os, (uint32_t)m_n);

        // Write a "TBLU" chunk.
        write_chunk(os, "TBLU", tblu_size);
        write_data(os, m_table, tblu_size - CHUNKSIZE);

        // Write a chunk for the double array.
        write_chunk(os, doublearray_traits::chunk_id(), sda_size);
        write_data(os, &m_da[0], sda_size - CHUNKSIZE);

        // Write a chunk for the tail array.
        write_chunk(os, "TAIL", tail_size);
        write_data(os, m_tail.block(), tail_size - CHUNKSIZE);
    }

protected:
    void write_uint32(std::ostream& os, uint32_t value)
    {
        write_data(os, &value, sizeof(value));
    }

    void write_data(std::ostream& os, const void *data, size_t size)
    {
        os.write(reinterpret_cast<const char*>(data), size);
    }

    void write_chunk(std::ostream& os, const char *chunk, size_type size)
    {
        os.write(chunk, 4);
        write_uint32(os, (uint32_t)size);
    }
};

/**
 * Empty type.
 *  Specify this class as a value type of dastrie::trie and dastrie::builder
 *  to make these class behave as a set (e.g., std::set) rather than a map.
 */
struct empty_type
{
    empty_type(int v = 0)
    {
    }

    friend dastrie::itail& operator>>(dastrie::itail& is, empty_type& obj)
    {
        return is;
    }

    friend dastrie::otail& operator<<(dastrie::otail& os, const empty_type& obj)
    {
        return os;
    }
};


};

/** @} */

/**
@mainpage Static Double Array Trie (DASTrie)

@section intro Introduction

Trie is a data structure of ordered tree that implements an associative array.
Looking up a record key (usually a string) is very efficient, which takes
<i>O(1)</i> with respect to the number of stored records <i>n</i>. Trie is
also known for efficient prefix matching, where the retrieved key strings are
the prefixes of a given query string.

Double-array trie, which was proposed by Jun-ichi Aoe in the late 1980s,
represents a trie in two parallel arrays (BASE and CHECK). Reducing the
storage usage drastically, double array tries have been used in practical
applications such as morphological analysis, spelling correction, and
Japanese Kana-Kanji convertion.

Static Double Array Trie (DASTrie) is an implementation of static double-array
trie. For the simplicity and efficiency, DASTrie focuses on building a
<i>static</i> double array from a list of records sorted by dictionary order
of keys. DASTrie does not provide the functionality for updating an existing
trie, whereas the original framework of double array supports dynamic updates.
DASTrie provides several features:
- <b>Associative array.</b> DASTrie is designed to store associations between
  key strings and their values (similarly to \c std::map). Thus, it is very
  straightforward to put and obtain the record values for key strings, while
  some implementations just return unique integer identifiers for key strings.
  It is also possible to omit record values so that DASTrie behaves as a
  string set (e.g., \c std::set).
- <b>Configurable value type.</b> A type of record values is configurable by
  a template argument of dastrie::trie and dastrie::builder. Any basic type
  (e.g., \c int, \c double) and strings (\c char* and \c std::string) can be
  used as a value type of records without any additional effort. User-defined
  types can also be used as record values only if two operators for
  serialization (\c operator<<() and \c operator>>()) are implemented.
- <b>Flexible key type.</b> A type of record keys is configurable by a
  template argument for dastrie::builder. One can choose either
  null-terminated C strings (char*) or C++ strings (std::string).
- <b>Fast look-ups.</b> Looking up a key takes <i>O(1)</i> with respect to the
  number of records <i>n</i>.
- <b>Prefix match.</b> DASTrie supports prefix matching, where the retrieved
  key strings are prefixes of a given query string. One can enumerate records
  of prefixes by using dastrie::trie::prefix_cursor.
- <b>Compact double array.</b> DASTrie implements double arrays whose each
  element is only 4 or 5 bytes long, whereas most implementations consume 8
  bytes for an double-array element. The size of double-array elements is
  configurable by trait classes, dastrie::doublearray4_traits and
  dastrie::doublearray5_traits.
- <b>Minimal prefix double-array.</b> DASTrie manages what is called a
  <i>tail array</i> so that non-branching suffixes do not waste the storage
  space of double array. This feature makes tries compact, improveing the
  storage utilization greatly.
- <b>Simple write interface.</b> DASTrie can serialize a trie data structure
  to C++ output streams (\c std::ostream) with dastrie::builder::write()
  function. Serialized data can be embedded into files with other arbitrary
  data.
- <b>Simple read interface.</b> DASTrie can prepare a double-array trie from
  an input stream (\c std::istream) (with dastrie::trie::read() function) or
  from a memory block (with dastrie::trie::assign() function) to which a
  serialized data is read or memory-mapped from a file.
- <b>Cross platform.</b> The source code can be compiled on Microsoft Visual
  Studio 2008, GNU C Compiler (gcc), etc.
- <b>Simple C++ implementation.</b> Following the good example of
  <a href="http://chasen.org/~taku/software/darts/">Darts</a>, DASTrie is
  implemented in a single header file (dastrie.h); one can use the DASTrie API
  only by including dastrie.h in a source code.

@section download Download

- <a href="http://www.chokkan.org/software/dist/dastrie-1.0.tar.gz">Source code</a>
- <a href="http://www.chokkan.org/software/dist/dastrie-1.0_win32.zip">Win32 binary</a>

DASTrie is distributed under the term of the
<a href="http://www.opensource.org/licenses/bsd-license.php">modified BSD license</a>.

@section changelog History
- Version 1.0 (2008-11-03):
	- Initial release.

@section sample Sample code

@include sample.cpp

@section tutorial Tutorial

@subsection tutorial_preparation Preparation

Put the header file "dastrie.h" to a INCLUDE path, and include the file to a
C++ source code. It's that simple.
@code
#include <dastrie.h>
@endcode

@subsection tutorial_builder_type Customizing a builder type

First of all, we need to design the types of records (keys and values) for
a trie, and derive a specialization of a builder. Key and record types can
be specified by the first and second template arguments of dastrie::builder,
@code
dastrie::builder<key_type, value_type, traits_type>
@endcode

A key type can be either \c char* or
\c std::string for your convenience. The string class \c std::string is
usually more convenient than \c char*, but some may prefer \c char* for
efficiency, e.g., allocating a single memory block that can store all of keys
and reading keys from a file at a time.

If you would like dastrie::trie to behave like \c std::set, use
dastrie::empty_type as a value type, which is a dummy value type that
serializes nothing with a tail array. This is an example of a trie builder
without values in which keys are represented by \c std::string,
@code
typedef dastrie::builder<std::string, dastrie::empty_type> builder_type;
@endcode

If you would like to use dastrie::trie in a similar manner as \c std::set,
specify a value type to the second template argument. This is an example of
a trie builder whose keys are \c char* and values are \c double,
@code
typedef dastrie::builder<char*, double> builder_type;
@endcode

DASTrie supports the following types as a value type    :
\c bool, \c short, \c unsigned \c short, \c int, \c unsigned \c int,
\c long, \c unsigned \c long, \c float, \c double, \c long \c double,
\c char*, \c std::string.
In addition, it is possible to use a user-defined type as a value type for a
trie. In order to do this, you must define \c operator<<() and \c operator>>()
for serializing values with a tail array. This is an example of a value type
(\c string_array) that holds an array (vector) of strings and reads/writes the
number of strings and their actual strings from/to a tail array,
@code
class string_array : public std::vector<std::string>
{
public:
    friend dastrie::itail& operator>>(dastrie::itail& is, string_array& obj)
    {
        obj.clear();

        uint32_t n;
        is >> n;
        for (uint32_t i = 0;i < n;++i) {
            std::string dst;
            is >> dst;
            obj.push_back(dst);
        }

        return is;
    }

    friend dastrie::otail& operator<<(dastrie::otail& os, const string_array& obj)
    {
        os << (uint32_t)obj.size();
        for (size_t i = 0;i < obj.size();++i) {
            os << obj[i];
        }
        return os;
    }
};

typedef dastrie::builder<std::string, string_array> builder_type;
@endcode

The third template argument (\c traits_type) of a builder class is to
customize the size of double-array elements. By default, DASTrie uses 5 bytes
per double-array element, which can store 0x7FFFFFFF elements in a trie. This
footprint is already smaller than other double-array implementations, you can
choose 4 bytes per element and save 1 byte per element if your data is small
enough to be stored with no longer than 0x007FFFFF elements (<i>note that the
number of elements is different from the number of records</i>). Specify
dastrie::doublearray4_traits at the third argument for implementing a double
array with 4 bytes per element.

@subsection tutorial_builder Building a trie

After designing a builder class, prepare records to be stored in a trie.
You can use dastrie::builder::record_type to define a record type.
@code
typedef builder_type::record_type record_type;
@endcode

An instance of dastrie::builder::record_type consists of two public member
variables, dastrie::builder::record_type::key and
dastrie::builder::record_type::value.
Records can be represented by an array of records, e.g.,
@code
record_type records[10] = {
    {"eight", 8},   {"five", 5},    {"four", 4},    {"nine", 9},
    {"one", 1},     {"seven", 7},   {"six", 6},     {"ten", 10},
    {"three", 3},   {"two", 2},
};
@endcode
or a vector of records, e.g.,
@code
std::vector<record_type> records;
@endcode

Make sure that records are sorted by dictionary order of keys. It may be a
good idea to use STL's \c sort() function if you have unsorted records.

Now you are ready to build a trie. Instantiate the builder class,
@code
builder_type builder;
@endcode
If necessary, set a callback function by using dastrie::builder::set_callback()
to receive progress reports in building a trie. Refer to dastrie_build.cpp for
an actual usage.

Call dastrie::builder::build to build a trie from records. The first argument
is the iterator (pointer) addressing to the first record, and the second
argument is the iterator (pointer) addressing one past the final record.
@code
builder.build(records, records + 10);
@endcode

You can store the newly-built trie to a file by using dastrie::builder::write.
This method outputs the trie to a binary stream (\c std::ostream).
@code
std::ofstream ofs("sample.db", std::ios::binary);
builder.write(ofs);
@endcode

@subsection tutorial_retrieve Accessing a trie

The class dastrie::trie provides the read access to a trie. The first template
argument specifies the value type of a trie, and the second argument customizes
the double array.
@code
dastrie::trie<value_type, traits_type>
@endcode
The value type and traits for dastrie::trie are required to be the same as
those specified for dastrie::builder.

This example defines a trie class \c trie_type that can access records whose
values are represented by \c int.
@code
typedef dastrie::trie<int> trie_type;
@endcode

The class dastrie::trie can read a trie structure from an input stream; the
dastrie::trie::read() function prepares a trie from an input stream
(\c std::istream). This example reads a trie from an input stream.
@code
std::ifstream ifs("sample.db", std::ios::binary);
trie_type trie;
trie.read(ifs);
@endcode

Alternatively, you can read a trie structure from a memory block by using the
dastrie::trie::assign() function. This function may be useful when you would
like to use mmap() API for reading a trie.

Now you are ready to access the trie. Please refer to the
@ref sample "sample code" for
retrieving a record (dastrie::trie::get() and dastrie::trie::find()),
checking the existence of a record (dastrie::trie::in()),
and retrieving records that are prefixes of keys (dastrie::trie::prefix()).

@section api Documentation

- @ref dastrie_api "DASTrie API"

@section performance Performance

This table shows the elapsed time for constructing a trie (write time), the
average time for processing a query (read time), and the size of the trie
generated for Google Web 1T 5-gram Version 1. In this experiment, all of
13,588,391 unigrams (125,937,836 bytes) in the Google Web 1T corpus are
inserted to the trie. Write time is measured by the total time elapsed for
inserting all unigrams (without frequency information) as key strings.
Read time is defined as the average elapsed time for a trie system to search
a unigram.

The read/write access was extremely faster than those of other database
libraries. The database was smaller than half the size of those generated by
other libraries.
This results suggest that the CQDB has the substantial advantage over the
existing database libraries for implementing a static quark database.

<table>
<tr>
<th>Implementation</th><th>Parameters</th><th>Write time [sec]</th><th>Read time [sec]</th><th>Database size [MB]</th>
</tr>
<tr align="right">
<td align="left">DASTrie 1.0</td>
<td align="left">UR = 0.95</td>
<td><b>1.48</b></td><td><b>0.65</b></td><td><b>35.2</b></td>
</tr>
<tr align="right">
<td align="left">DASTrie 1.0</td>
<td align="left">UR = 0.50</td>
<td><b>1.48</b></td><td><b>0.65</b></td><td><b>35.2</b></td>
</tr>
<tr align="right">
<td align="left">darts 0.32</td>
<td align="left">Default</td>
<td>91.8</td><td>37.5</td><td>79.7</td>
</tr>
<tr align="right">
<td align="left">DynDA 0.01</td>
<td align="left">Default</td>
<td>95.4</td><td>80.6</td><td>76.3</td>
</tr>
<tr align="right">
<td align="left">TinyDA 1.23</td>
<td align="left">Default</td>
<td>95.4</td><td>80.6</td><td>76.3</td>
</tr>
<tr align="right">
<td align="left">Tx 0.12</td>
<td align="left">table_size=4000000</td>
<td>15.7</td><td>12.0</td><td>92.2</td>
</tr>
</table>

@section acknowledgements Acknowledgements
The data structure of the (static) double-array trie is described in:
- Jun-ichi Aoe. An efficient digital search algorithm by using a double-array structure. <i>IEEE Transactions on Software Engineering</i>, Vol. 15, No. 9, pp. 1066-1077, 1989.
- Susumu Yata, Masaki Oono, Kazuhiro Morita, Masao Fuketa, Toru Sumitomo, and Jun-ichi Aoe. A compact static double-array keeping character codes. <i>Information Processing and Management</i>, Vol. 43, No. 1, pp. 237-247, 2007.

@section reference References
- <a href="http://linux.thai.net/~thep/datrie/datrie.html">An Implementation of Double-Array Trie</a> by Theppitak Karoonboonyanan.
- <a href="http://chasen.org/~taku/software/darts/">Darts: Double-ARray Trie System</a> by Taku Kudo.
- <a href="http://nanika.osonae.com/DynDA/index.html">Dynamic Double-Array Library</a> by Susumu Yata.
- <a href="http://nanika.osonae.com/TinyDA/index.html">Tiny Double-Array Library</a> by Susumu Yata.
- <a href="http://www-tsujii.is.s.u-tokyo.ac.jp/~hillbig/tx.htm">Tx: Succinct Trie Data structure</a> by Daisuke Okanohara.

*/

#endif/*__DASTRIE_H__*/

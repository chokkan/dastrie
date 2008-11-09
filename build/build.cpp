/*
 *      A sample program for building a trie.
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

/* $Id$ */

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <dastrie.h>
#include <optparse.h>

class option : public optparse
{
public:
    enum {
        TYPE_EMPTY,
        TYPE_INT,
        TYPE_DOUBLE,
        TYPE_STRING,
    };

    int type;
    bool compact;
    std::string db;
    bool help;

public:
    option() : type(TYPE_EMPTY), compact(false), help(false)
    {
    }

    BEGIN_OPTION_MAP_INLINE()
        ON_OPTION_WITH_ARG(SHORTOPT('t') || LONGOPT("type"))
            if (strcmp(arg, "empty") == 0) {
                type = TYPE_EMPTY;
            } else if (strcmp(arg, "int") == 0) {
                type = TYPE_INT;
            } else if (strcmp(arg, "double") == 0) {
                type = TYPE_DOUBLE;
            } else if (strcmp(arg, "string") == 0) {
                type = TYPE_STRING;
            } else {
                std::stringstream ss;
                ss << "unknown record type specified: " << arg;
                throw invalid_value(ss.str());
            }

        ON_OPTION(SHORTOPT('c') || LONGOPT("compact"))
            compact = true;

        ON_OPTION_WITH_ARG(SHORTOPT('d') || LONGOPT("db"))
            db = arg;

        ON_OPTION(SHORTOPT('h') || LONGOPT("help"))
            help = true;

    END_OPTION_MAP()
};

static void usage(std::ostream& os, const char *argv0)
{
    os << "USAGE: " << argv0 << " [OPTIONS] INPUT" << std::endl;
    os << "This utility builds a double-array trie from an input file (INPUT)." << std::endl;
    os << std::endl;
    os << "  INPUT   an input file in which each line represents a record; a record (line)" << std::endl;
    os << "          consists of a key string and its value (optional) separated by a TAB" << std::endl;
    os << "          character; the records must be sorted by dictionary order of keys." << std::endl;
    os << std::endl;
    os << "OPTIONS:" << std::endl;
    os << "  -t, --type=TYPE    specify a type of record values:" << std::endl;
    os << "      empty              no values [DEFAULT]; the trie will store keys only" << std::endl;
    os << "      int                integer values" << std::endl;
    os << "      double             floating-point values" << std::endl;
    os << "      string             string values" << std::endl;
    os << "  -c, --compact      make a double array trie compact by storing a double-array" << std::endl;
    os << "                     element in 4 bytes; this compaction is available only when" << std::endl;
    os << "                     the number of records are small" << std::endl;
    os << "  -d, --db           specify a database file to which the double array trie will" << std::endl;
    os << "                     be stored; by default, this utility write no database" << std::endl;
    os << "  -h, --help         show this help message and exit" << std::endl;
}


static char* read_text(const char *filename, std::streamoff& size)
{
    // Open the input file.
    std::ifstream ifs(filename);
    if (ifs.fail()) {
        return NULL;
    }

    // Get the size of the input file.
    ifs.seekg(0, std::ios::end);
    size = (std::streamoff)ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    // Read the entire data of the input file.
    char *block = new char[size+1];
    std::memset(block, 0, sizeof(char) * (size+1));
    ifs.read(block, size);
    return block;
}

static size_t count_records(const char *block)
{
    size_t n = 0;
    const char *p = block;

    while (*p) {
        if (*p == '\n') {
            ++n;
        }
        ++p;
    }

    if (*block) {
        if (p[-1] != '\n') {
            ++n;
        }
    }

    return n;
}

inline static void init_value(dastrie::empty_type& value)
{
}

inline static void init_value(int& value)
{
    value = 0;
}

inline static void init_value(double& value)
{
    value = 0;
}

inline static void init_value(char*& value)
{
    static char *empty = "";
    value = empty;
}

inline static void set_value(char *p, dastrie::empty_type& value)
{
}

inline static void set_value(char *p, int& value)
{
    *p = 0;
    value = std::atoi(p+1);
}

inline static void set_value(char *p, double& value)
{
    *p = 0;
    value = std::atof(p+1);
}

inline static void set_value(char *p, char*& value)
{
    *p = 0;
    value = p+1;
}

template <class record_type>
static void set_records(
    record_type* records,
    size_t n,
    char *block
    )
{
    size_t i = 0;
    char *p = block;

    while (i < n) {
        if (records[i].key == NULL) {
            records[i].key = p;
            init_value(records[i].value);
        }
        if (*p == 0) {
            break;
        } else if (*p == '\t') {
            set_value(p, records[i].value);
        } else if (*p == '\n') {
            *p = 0;
            ++i;
        }
        ++p;
    }
}

class progress
{
protected:
    std::ostream& m_os;
    int m_prev;

public:
    progress(std::ostream& os) : m_os(os), m_prev(-1)
    {
    }

    virtual ~progress()
    {
    }

    static void callback(void *instance, size_t i, size_t n)
    {
        reinterpret_cast<progress*>(instance)->report((int)(i * 100. / n));
    }

    void report(int current)
    {
	    while (m_prev < current) {
		    ++m_prev;
		    if (m_prev % 2 == 0) {
			    if (m_prev % 10 == 0) {
                    m_os << m_prev / 10;
                    m_os.flush();
			    } else {
                    m_os << ".";
                    m_os.flush();
			    }
		    }
	    }
    }
};

template <class value_type, class traits_type>
int build(char *text, size_t size, const option& opt)
{
    typedef dastrie::builder<char*, value_type, traits_type> builder_type;
    typedef typename builder_type::record_type record_type;

    std::ostream& os = std::cout;
    std::ostream& es = std::cerr;

    // Count the number of records in the input text.
    size_t n = count_records(text);
    if (n == 0) {
        es << "ERROR: No records in the input data." << std::endl;
        return 1;
    }

    // Allocate an array of records.
    record_type* records = new record_type[n];
    std::memset(records, 0, sizeof(record_type) * n);

    // Set records from the input text.
    set_records(records, n, text); 

    os << "Size of input text: " << size << std::endl;
    os << "Number of records: " << n << std::endl;
    os << std::endl;

    // Build a double-array trie.
    builder_type builder;
    try {
        progress prog(os);
        builder.set_callback(&prog, prog.callback);
        os << "Building a double array trie..." << std::endl;
        builder.build(records, records + n);
        os << std::endl << std::endl;
    } catch (const typename builder_type::exception& e) {
        // Abort if something went wrong...
        os << std::endl << std::endl;
        es << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    // Report the statistics of the trie.
    const typename builder_type::stat_type& stat = builder.stat();
    os << "[Double array]" << std::endl;
    os << "Size in bytes: " << stat.da_size << std::endl;
    os << "Number of nodes: " << stat.da_num_nodes << std::endl;
    os << "Number of leaves: " << stat.da_num_leaves << std::endl;
    os << "Number of elements: " << stat.da_num_total << std::endl;
    os << "Number of elements used: " << stat.da_num_used << std::endl;
    os << "Storage utilization: " << stat.da_usage << std::endl;
    os << "Average number of trials for finding bases: " << stat.bt_avg_base_trials << std::endl;
    os << "[Tail array]" << std::endl;
    os << "Size in bytes: " << stat.tail_size << std::endl;
    os << std::endl;

    // Write the database.
    if (!opt.db.empty()) {
        std::ofstream ofs;
        ofs.open(opt.db.c_str(), std::ios::binary);
        builder.write(ofs);
    }

    return 0;    
}

int main(int argc, char *argv[])
{
    option opt;
    int ret = 0;
    int arg_used = 0;
    std::ostream& es = std::cerr;
    std::ostream& os = std::cout;

    // Show the copyright information.
    os << "DASTrie builder ";
    os << DASTRIE_MAJOR_VERSION << "." << DASTRIE_MINOR_VERSION << " ";
    os << DASTRIE_COPYRIGHT << std::endl;
    os << std::endl;

    // Parse the command-line options.
    try { 
        arg_used = opt.parse(argv, argc);
    } catch (const optparse::unrecognized_option& e) {
        es << "ERROR: unrecognized option: " << e.what() << std::endl;
        return 1;
    } catch (const optparse::invalid_value& e) {
        es << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    // Show the help message and exit.
    if (opt.help) {
        usage(os, argv[0]);
        return ret;
    }

    // Make sure that an input file is specified.
    if (argc <= arg_used) {
        es << "ERROR: No input file specified." << std::endl;
        return 1;
    }

    // Read the source data.
    std::streamoff textsize;
    char *text = read_text(argv[arg_used], textsize);
    if (text == NULL) {
        es << "ERROR: Failed to read the input data." << std::endl;
        return 1;
    }

    switch (opt.type) {
    case option::TYPE_EMPTY:
        if (opt.compact) {
            return build<
                dastrie::empty_type,
                dastrie::doublearray4_traits
            >(text, (size_t)textsize, opt);
        } else {
            return build<
                dastrie::empty_type,
                dastrie::doublearray5_traits
            >(text, (size_t)textsize, opt);
        }
    case option::TYPE_INT:
        if (opt.compact) {
            return build<
                int,
                dastrie::doublearray4_traits
            >(text, (size_t)textsize, opt);
        } else {
            return build<
                int,
                dastrie::doublearray5_traits
            >(text, (size_t)textsize, opt);
        }
    case option::TYPE_DOUBLE:
        if (opt.compact) {
            return build<
                double,
                dastrie::doublearray4_traits
            >(text, (size_t)textsize, opt);
        } else {
            return build<
                double,
                dastrie::doublearray5_traits
            >(text, (size_t)textsize, opt);
        }
    case option::TYPE_STRING:
        if (opt.compact) {
            return build<
                char*,
                dastrie::doublearray4_traits
            >(text, (size_t)textsize, opt);
        } else {
            return build<
                char*,
                dastrie::doublearray5_traits
            >(text, (size_t)textsize, opt);
        }
    }

    return 0;
}

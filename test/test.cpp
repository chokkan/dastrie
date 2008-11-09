/*
 *      A sample program for testing a trie.
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

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <dastrie.h>
#include <optparse.h>

class option : public optparse
{
public:
    bool compact;
    std::string db;
    bool help;

public:
    option() : compact(false), help(false)
    {
    }

    BEGIN_OPTION_MAP_INLINE()
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

template <class value_type, class traits_type>
int test(char *text, size_t size, const option& opt)
{
    typedef dastrie::trie<char*, traits_type> trie_type;
    trie_type trie;
    char *p = text, *key = NULL;
    std::ostream& os = std::cout;
    std::ostream& es = std::cerr;

    if (opt.db.empty()) {
        es << "ERROR: No database file specified." << std::endl;
        return 1;
    }

    std::ifstream ifs(opt.db.c_str(), std::ios::binary);
    if (ifs.fail()) {
        es << "ERROR: Database file not found." << std::endl;
        return 1;
    }

    if (trie.read(ifs) == 0) {
        es << "ERROR: Failed to read the database." << std::endl;
        return 1;
    }

    key = p;
    while (*p) {
        if (*p == '\n' || *p == 0) {
            *p = 0;
            if (!trie.in(key)) {
                es << "ERROR: The key not found, " << key << std::endl;
            }
            key = p+1;
        }
        if (*p == 0) {
            break;
        }
        ++p;
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
    es << "DASTrie tester ";
    es << DASTRIE_MAJOR_VERSION << "." << DASTRIE_MINOR_VERSION << " ";
    es << DASTRIE_COPYRIGHT << std::endl;
    es << std::endl;

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

    // Dispatch.
    if (opt.compact) {
        return test<
            dastrie::empty_type,
            dastrie::doublearray4_traits
        >(text, (size_t)textsize, opt);
    } else {
        return test<
            dastrie::empty_type,
            dastrie::doublearray5_traits
        >(text, (size_t)textsize, opt);
    }

    return 0;
}

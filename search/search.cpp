/*
 *      A sample program for retrieving records in a trie.
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

    enum {
        MODE_SEARCH,
        MODE_CHECK,
        MODE_PREFIX,
        MODE_HELP,
    };

    int type;
    int mode;
    bool compact;
    std::string db;

public:
    option() : type(TYPE_EMPTY), mode(MODE_SEARCH), compact(false)
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

        ON_OPTION(SHORTOPT('i') || LONGOPT("in"))
            mode = MODE_CHECK;

        ON_OPTION(SHORTOPT('p') || LONGOPT("prefix"))
            mode = MODE_PREFIX;

        ON_OPTION(SHORTOPT('h') || LONGOPT("help"))
            mode = MODE_HELP;

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

inline static std::ostream& output_value(std::ostream& os, const dastrie::empty_type& value)
{
    return os;
}

inline static std::ostream& output_value(std::ostream& os, const int& value)
{
    os << value;
    return os;
}

inline static std::ostream& output_value(std::ostream& os, const double& value)
{
    os << value;
    return os;
}

inline static std::ostream& output_value(std::ostream& os, const char* value)
{
    os << value;
    return os;
}

template <class value_type, class traits_type>
int search(const option& opt)
{
    typedef dastrie::trie<value_type, traits_type> trie_type;
    trie_type trie;
    std::istream& is = std::cin;
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

    for (;;) {
        std::string line;
        std::getline(is, line);
        if (is.eof()) {
            break;
        }

        switch (opt.mode) {
        case option::MODE_SEARCH:
            {
                value_type value;
                if (trie.find(line.c_str(), value)) {
                    os << line << '\t';
                    output_value(os, value) << std::endl;
                }
            }
            break;
        case option::MODE_CHECK:
            if (trie.in(line.c_str())) {
                os << line << "\t1" << std::endl;
            } else {
                os << line << "\t0" << std::endl;
            }
            break;
        case option::MODE_PREFIX:
            {
                typename trie_type::prefix_cursor pfx = trie.prefix(line.c_str());
                while (pfx.next()) {
                    os << pfx.query.substr(0, pfx.length) << '\t';
                    output_value(os, pfx.value) << std::endl;
                }
            }
            break;
        }
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
    es << "DASTrie search ";
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
    if (opt.mode == option::MODE_HELP) {
        usage(os, argv[0]);
        return ret;
    }

    // Dispatch.
    switch (opt.type) {
    case option::TYPE_EMPTY:
        if (opt.compact) {
            return search<
                dastrie::empty_type,
                dastrie::doublearray4_traits
            >(opt);
        } else {
            return search<
                dastrie::empty_type,
                dastrie::doublearray5_traits
            >(opt);
        }
    case option::TYPE_INT:
        if (opt.compact) {
            return search<
                int,
                dastrie::doublearray4_traits
            >(opt);
        } else {
            return search<
                int,
                dastrie::doublearray5_traits
            >(opt);
        }
    case option::TYPE_DOUBLE:
        if (opt.compact) {
            return search<
                double,
                dastrie::doublearray4_traits
            >(opt);
        } else {
            return search<
                double,
                dastrie::doublearray5_traits
            >(opt);
        }
    case option::TYPE_STRING:
        if (opt.compact) {
            return search<
                char*,
                dastrie::doublearray4_traits
            >(opt);
        } else {
            return search<
                char*,
                dastrie::doublearray5_traits
            >(opt);
        }
    }

    return 0;
}

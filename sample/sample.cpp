#include <fstream>
#include <iostream>
#include <string>
#include <dastrie.h>

int main(int argc, char *argv[])
{
    typedef dastrie::builder<char*, int> builder_type;
    typedef dastrie::trie<int> trie_type;
    typedef builder_type::record_type record_type;

    // Records sorted by dictionary order of keys.
    record_type records[] = {
        {"eight", 8},   {"five", 5},    {"four", 4},    {"nine", 9},
        {"one", 1},     {"seven", 7},   {"six", 6},     {"ten", 10},
        {"three", 3},   {"two", 2},
    };

    try {
        // Build a double-array trie from the records.
        builder_type builder;
        builder.build(records, records + sizeof(records)/sizeof(records[0]));

        // Store the double-array trie to a file.
        std::ofstream ofs("sample.db", std::ios::binary);
        builder.write(ofs);
        ofs.close();

    } catch (const builder_type::exception& e) {
        // Abort if something went wrong...
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    // Open the trie file.
    std::ifstream ifs("sample.db", std::ios::binary);
    if (ifs.fail()) {
        std::cerr << "ERROR: Failed to open a trie file." << std::endl;
        return 1;
    }

    // Read the trie.
    trie_type trie;
    if (trie.read(ifs) == 0) {
        std::cerr << "ERROR: Failed to read a trie file." << std::endl;
        return 1;
    }

    /*
      Note that, although this sample program uses a file, a trie class can
      also receive a double-array trie directly from a builder,
        trie.assign(builder.doublearray(), builder.tail(), builder.table());
    */

    // Get the values of keys or the default value if the key does not exist.
    std::cout << trie.get("one", -1) << std::endl;              // 1
    std::cout << trie.get("other", -1) << std::endl;            // -1

    // Check the existence of a key and obtain its value.
    int value;
    if (trie.find("two", value)) {
        std::cout << value << std::endl;                        // 2
    }

    // Check the existence of keys.
    std::cout << trie.in("ten") << std::endl;                   // 1 (true)
    std::cout << trie.in("eleven") << std::endl;                // 0 (false)

    // Get records whose keys are prefixes of "eighteen".
    trie_type::prefix_cursor pfx = trie.prefix("eighteen");
    while (pfx.next()) {
        std::cout
            << pfx.query.substr(0, pfx.length) << " "           // eight
            << pfx.value << std::endl;                          // 8
    }

    return 0;
}

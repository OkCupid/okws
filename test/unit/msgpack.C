// -*-c++-*-
//-----------------------------------------------------------------------

#include "async.h"
#include "pub3.h"
#include "pub3msgpack.h"
using namespace pub3;

struct test_case_t {
    ptr<expr_t> pub;
    str packed;
    bool test_pack;

    test_case_t(ptr<expr_t> _pub, const char *s, size_t n, bool _tp=true) : pub(_pub), test_pack(_tp) {
        packed.setbuf(s, n);
    }

};

// Tests are a modified version of test_case.py from msgpack-python
// https://github.com/msgpack/msgpack-python
vec<test_case_t> test_cases() {
    vec<test_case_t> cases = vec<test_case_t>();
    // TESTS 0-4
    cases.push_back(test_case_t(expr_null_t::alloc(), "\xc0", 1));
    cases.push_back(test_case_t(expr_bool_t::alloc(false), "\xc2", 1));
    cases.push_back(test_case_t(expr_bool_t::alloc(true), "\xc3", 1));
    cases.push_back(test_case_t(expr_int_t::alloc(0), "\x00", 1));
    cases.push_back(test_case_t(expr_int_t::alloc(127), "\x7f", 1));
    // TESTS 5-9
    cases.push_back(test_case_t(expr_int_t::alloc(128), "\xcc\x80", 2));
    cases.push_back(test_case_t(expr_int_t::alloc(256), "\xcd\x01\x00", 3));
    cases.push_back(test_case_t(expr_int_t::alloc(-1), "\xff", 1));
    cases.push_back(test_case_t(expr_int_t::alloc(-33), "\xd0\xdf", 2));
    cases.push_back(test_case_t(expr_int_t::alloc(-129), "\xd1\xff\x7f", 3));
    // TESTS 10-15
    cases.push_back(test_case_t(expr_double_t::alloc(1.0), "\xcb\x3f\xf0\x00\x00\x00\x00\x00\x00", 9));
    cases.push_back(test_case_t(expr_list_t::alloc(), "\x90", 1));
    cases.push_back(test_case_t(expr_dict_t::alloc(), "\x80", 1));
    cases.push_back(test_case_t(expr_str_t::alloc(""), "\xa0", 1));
    cases.push_back(test_case_t(expr_str_t::alloc("abcdefghijklmnopqrstuvwxyz01234"), "\xbf""abcdefghijklmnopqrstuvwxyz01234", 32));
    cases.push_back(test_case_t(expr_str_t::alloc("abcdefghijklmnopqrstuvwxyz012345"), "\xda\x00\x20""abcdefghijklmnopqrstuvwxyz012345", 35));

    ptr<expr_list_t> list1 = expr_list_t::alloc();
    ptr<expr_list_t> list2 = expr_list_t::alloc();
    for (int i = 0; i < 16; i++) {
        if (i < 15) { list1->push_back(expr_int_t::alloc(i)); }
        list2->push_back(expr_int_t::alloc(i));
    }
    // TESTS 16-17
    cases.push_back(test_case_t(list1, "\x9f\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e", 16));
    cases.push_back(test_case_t(list2, "\xdc\x00\x10\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f", 19));

    ptr<expr_dict_t> dict1 = expr_dict_t::alloc();
    ptr<expr_dict_t> dict2 = expr_dict_t::alloc();
    ptr<expr_dict_t> dict3 = expr_dict_t::alloc();
    ptr<expr_dict_t> dict4 = expr_dict_t::alloc();

    dict1->insert("", int64_t(1));
    for (int i = 0; i < 16; i++) {
        if (i < 15) { dict2->insert((str("") << i), int64_t(i)); }
        dict3->insert((str("") << i), int64_t(i));
    }
    // TEST 18-21
    cases.push_back(test_case_t(dict1, "\x81\xa0\x01", 3));
    cases.push_back(test_case_t(dict2, "\x8f\x00\x00\x01\x01\x02\x02\x03\x03\x04\x04\x05\x05\x06\x06\x07\x07\x08\x08\t\t\n\n\x0b\x0b\x0c\x0c\r\r\x0e\x0e", 31, false));
    cases.push_back(test_case_t(dict3, "\xde\x00\x10\x00\x00\x01\x01\x02\x02\x03\x03\x04\x04\x05\x05\x06\x06\x07\x07\x08\x08\t\t\n\n\x0b\x0b\x0c\x0c\r\r\x0e\x0e\x0f\x0f", 35, false));

    return cases;
}

void show(str s) { 
    for (int i = 0; i < s.len(); i++) {
        printf("%02x ", uint8_t(s[i]));
    }
    printf("\n");
}

int check(size_t test_num, test_case_t c) {
    str s = msgpack::encode(c.pub);
    int failures = 0;
    if (c.test_pack) {
        if (!s) {
            warn << "FAILED PACK TEST " << test_num << ", empty string!\n";
            failures++;
        }
        else if (s != c.packed) {
            warn << "FAILED PACK TEST " << test_num << "\n";
            show(s);
            show(c.packed);
            failures++;
        }
    }
    ptr<expr_t> unpacked = msgpack::decode(c.packed);
    if (!unpacked) {
        warn << "FAILED UNPACK TEST " << test_num << ", failed to decode!\n";
        failures++;
    // Janky and sad comparison =(
    } else if (unpacked->to_str() != c.pub->to_str()) {
        warn << "FAILED UNPACK TEST " << test_num << "\n";
        warn << unpacked->to_str() << "\n";
        warn << c.pub->to_str() << "\n";
        failures++;
    }

    return failures;
}

int check_dict_packing() {
    int failures = 0;
    ptr<expr_dict_t> dict2 = expr_dict_t::alloc();
    ptr<expr_dict_t> dict3 = expr_dict_t::alloc();
    for (int i = 0; i < 16; i++) {
        if (i < 15) { dict2->insert((str("") << i), int64_t(i)); }
        dict3->insert((str("") << i), int64_t(i));
    }

    str s = msgpack::encode(dict2);
    if (!s) { 
        warn << "FAILED DICT PACK TEST 1, empty string!\n";
        failures++;
    } else if (s[0] != '\x8f') {
        warn << "FAILED DICT PACK TEST 1\n";
        show(s);
        failures++;
    }

    s = msgpack::encode(dict3);
    if (!s) { 
        warn << "FAILED DICT PACK TEST 2, empty string!\n";
        failures++;
    } else if (s[0] != '\xde' || s[1] != '\x00' || s[2] != '\x10') {
        warn << "FAILED DICT PACK TEST 2\n";
        show(s);
        failures++;
    }
    return failures;
}

int 
main (int argc, char *argv[])
{
    setprogname (argv[0]);
    vec<test_case_t> tcs = test_cases();
    int failures = 0;
    for (size_t i = 0; i < tcs.size(); ++i) {
        failures += check(i, tcs[i]);
    }
    failures += check_dict_packing();
    if (failures) {
        warn << "FAILED " << failures << " TESTS\n";
    } else {
        warn << "all tests ok\n";
    }
}




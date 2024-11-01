//
// hash_helper.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
#include <cstring>
#include <openssl/md5.h>

size_t hostname_to_machine(const char *hostname, int numberOfMachine)
{
    unsigned char md[MD5_DIGEST_LENGTH];
    size_t s = strlen(hostname);
    MD5((const unsigned char *)hostname, s, md);
    uint64_t first;
    memcpy(&first, md, sizeof(first));
    return size_t(first % numberOfMachine);
}

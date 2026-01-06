# stb_swiss_tbl
A simple swiss table implementation based on google's fast swiss table from this [talk](https://www.youtube.com/watch?v=ncHmEUmJZf4&t=1912s)

## Why use this library?
-  You want to avoid introducing cmake/bazel to your build scripts.
-  You need a stb style library for c and don't want to compile [abseil](https://github.com/abseil/abseil-cpp) to object file.

## Usage

```c
#include <stdio.h>
#define ST_IMPLEMENTATION
#include "stb_swiss_tbl.h"

int main() {
    // Now use it:
    swiss_tbl* map = st_new();
    st_str key = {(char*)"hello", 5};
    map = st_insert(map, &key, 42);
    
    // Finding element
    int64_t out;
    if (st_find(map, &key, &out)) {
        printf("Found %s = %ld", key.data, out);
    }
}
```
### Options:
- **ST_MALLOC**: Malloc function for allocation (default is stdlib.h `malloc`)
- **ST_FREE**: Function for freeing allocation above (default is stdlib.h `free`)
- **ST_MEMCMP**: Memcmp function (default is string.h `memcmp`)
- **ST_PROBE_FUNC**: Function to use for probing (default is linear probe); Signature is `size_t (size_t group, size_t number_of_groups)` 
- **ST_HASH_FUNC**: Hashing function (default is `stb_hash_bytes`; would suggest [**rapidhash**](https://github.com/Nicoshev/rapidhash/blob/master/rapidhash.h) for better performance); Signature is `size_t (char* key_bytes, size_t key_size)`
- **ST_NO_DEF_STB_DS_IMPLEMENTATION**: Whether to not define **STB_DS_IMPLEMENTATION** with the **ST_IMPLEMENTATION** block (help for implementation collision).
## Dependencies:
- [**stb_ds**](https://github.com/nothings/stb/blob/master/stb_ds.h): Array implementation for buckets/slots and default hash function.
## Important
-  This implementation might have some functionalities missing that are present in abseil above.


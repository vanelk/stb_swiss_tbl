# stb_swiss_tbl
A simple swiss table implementation based on google's fast swiss table from this [talk](https://www.youtube.com/watch?v=ncHmEUmJZf4&t=1912s)

## Why use this library?
-  You want to avoid introducing cmake/bazel to your build scripts.
-  You need a stb style library for c and don't want to compile [abseil](https://github.com/abseil/abseil-cpp) to object file.

## Usage

```c
#define ST_IMPLEMENTATION
#include "stb_swiss_tbl.h"

// Now use it:
st_map* map = st_new();
st_str key = {(uint8_t*)"hello", 5};
map = st_insert(map, &key, 42);

// Finding element
int64_t out;
if (st_find(&key, &out)) {
    printf("Found hello = %ld", out);
}
```
### Options:
- **ST_MALLOC**: Malloc function for allocation (default is stdlib `malloc`)
- **ST_FREE**: Free function for allocation (default is stdlib `free`)
- **ST_MEMCMP**: Memcmp function for allocation (default is string.h `memcmp`)
- **ST_PROBE_FUNC**: Function to use for probing takes in the (`group`, `number_of_groups`) (default is linear probe)
- **ST_HASH_FUNC**: Hashing function (default is `rapidhash`)

## Dependencies:
- [**stb_ds**](https://github.com/nothings/stb/blob/master/stb_ds.h): Array implementation for buckets/slots.
- [**rapidhash**](https://github.com/Nicoshev/rapidhash/blob/master/rapidhash.h): Default hash implementation.
## Important
-  This implementation might have some functionalities missing that are present in abseil above.


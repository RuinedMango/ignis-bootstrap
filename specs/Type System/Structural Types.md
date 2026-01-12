### enum
Defines a new type whose possible values are an ordered list of set values.

### struct
Defines a structure of field's, declarations, and function definitions. Field's can have default values. Struct's can be packed to integer sizes.
#### pod
Default struct type
#### unique
struct with forced move semantics
#### capsule
struct with a forced destructor method
#### soa
simd optimized 

### union
Like a struct but only one field can be active at a time. Packable. zig-like tagged unions using enums for switch cases.
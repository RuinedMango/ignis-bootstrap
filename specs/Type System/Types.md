### u_ ?le || be
Unsigned arbitrary bit-width integer. Can specify endianness.

### i_ ?le || be
Signed arbitrary bit-width integer. Can specify endianness.

### f_ ?le || be
Arbitrary bit-width floating point value. Non-standard sizes are emulated in software making compute higher but can optimize memory. Can specify endianness.

### bool
A wrapper around u1 with semantic benefits.

### vec\<T\, \_\>
SIMD backed vector type of either integer or float values.

### mat\<T\, \_\, \_\>
SIMD backed matrix type of either integer or float values.

### quat\<T>
Quaternion of type T. A Vec4 with special math and semantic sauce.

### complex\<T>
Complex Number of type T. A vec2 with special math and some semantic sauce.

### \*T
Pointer to value of type T.

### \[\*]T
Pointer to an unknown amount of items.

### ?T
Optional type, can be null, NaN or purely unallocated.

### [len]T
An array of T.

### []T
Pointer to a runtime known amount of items.

## type
Type of types.

### noreturn
Return type of break, continue, return, unreachable, and while(true).

### void
Void value.

### opaqueptr
Used for type less pointers.

### anyerror
Placeholder for any error type.
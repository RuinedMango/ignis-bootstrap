For loops operate on arrays and slices. You can also iterate over multiple objects of the same length. You can also iterate over a range of numbers. They can also be unrolled and empowered at comptime with the inline keyword.
```
//operate over array or slice
for({1, 3, 5}) <item>{
	printf(item);
}

//operate over multiple arrays or slices
for({1, 3, 5}, {2, 4, 6}) <item1, item2>{
	printf(item1);
	printf(item2);
}

//operate over range
for(0..8) <i>{
	printf(i);
}

//unrolled at comptime
inline for(0..8) <i>{
	printf(i);
}
```
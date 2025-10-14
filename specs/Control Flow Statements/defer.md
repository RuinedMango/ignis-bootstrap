A defer statement defers the execution of a statement until the end of the scope it's in.
```
//prints 21
fn void: deferExample(){
	defer printf("1");
	printf("2");
	return;
}
```
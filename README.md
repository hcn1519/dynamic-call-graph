# dynamic-call-graph

```shell
valgrind --tool=callgrind --tree=both ./week1 ../pride-and-prejudice.txt
```

```shell
dot -Tpng ./output.dot -o output.png
```

```shell
gprof2dot -f callgrind callgrind.out.916  -o output916.dot
```

## Important consideration ##
The benchmark python script expects a compiled executable in this folder.

The recommended compilation from the project's root for the benchmark is:
```bash
gcc -O2 -fopenmp src/image.c tests/bench_mark.c -o build/image_processor
```

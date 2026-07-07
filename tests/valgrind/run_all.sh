#!/bin/bash

for f in *.py
do
if [[ "$f" == *"dsl"* ]]; then
    continue
fi

    echo
    echo
    echo "====== $f ======" 
    echo
    echo
	PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python3.13.supp python3d $f
done

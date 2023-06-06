import os, sys
from pathlib import Path

def createCallgrind():
    if len(sys.argv) < 1:
        return

    print("compile basic.cpp")
    os.system("g++ basic.cpp -o basic")

    print("compile coroutine.cpp")
    os.system("g++ --std=c++20 coroutine.cpp -o coroutine")

    filePath = Path(sys.argv[1])

    print(f"Create callgrind output $ ./basic {sys.argv[1]}")
    os.system(f"valgrind --tool=callgrind --callgrind-out-file=basic-{filePath.stem}.out ./basic {sys.argv[1]}")

    print(f"Create callgrind output $ ./coroutine {sys.argv[1]}")
    os.system(f"valgrind --tool=callgrind --callgrind-out-file=coroutine-{filePath.stem}.out ./coroutine {sys.argv[1]}")
   
createCallgrind()
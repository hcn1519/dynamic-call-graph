import os, sys
from pathlib import Path

def createImages():
    if len(sys.argv) < 1:
        return
    
    filePath = Path(sys.argv[1])
    
    print(f"gprof2dot -f callgrind basic-{filePath.stem}.out --strip --wrap -o basic-{filePath.stem}.dot")
    os.system(f"gprof2dot -f callgrind basic-{filePath.stem}.out --strip --wrap -o basic-{filePath.stem}.dot")

    print(f"gprof2dot -f callgrind coroutine-{filePath.stem}.out --strip --wrap -o coroutine-{filePath.stem}.dot")
    os.system(f"gprof2dot -f callgrind coroutine-{filePath.stem}.out --strip --wrap -o coroutine-{filePath.stem}.dot")

    format = ["dot", "fdp", "sfdp", "neato"]
    # format = ["dot"]

    for formatName in format:
        print(f"{formatName} -Tpng basic-{filePath.stem}.dot -o basic-{filePath.stem}-{formatName}.png")
        os.system(f"{formatName} -Tpng basic-{filePath.stem}.dot -o basic-{filePath.stem}-{formatName}.png")
        
        print(f"{formatName} -Tpng coroutine-{filePath.stem}.dot -o coroutine-{filePath.stem}-{formatName}.png")
        os.system(f"{formatName} -Tpng coroutine-{filePath.stem}.dot -o coroutine-{filePath.stem}-{formatName}.png")
        
createImages()
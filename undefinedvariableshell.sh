LLVM_BUILD="${HOME}/桌面/llvm-source-build/llvm-build"
cd ./cmake-build-debug
cmake -G Ninja -DLLVM_BUILD=${LLVM_BUILD} ..
ninja
cd ../tests/MainChecker
clang++ -emit-ast -c staticApptest.cpp
../../cmake-build-debug/tools/MainChecker/MainChecker astList.txt config.txt

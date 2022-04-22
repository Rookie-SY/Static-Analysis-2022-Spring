LLVM_BUILD="/home/cyrus/桌面/llvm-source-build/llvm-build"
cd ./cmake-build-debug
cmake -G Ninja -DLLVM_BUILD=${LLVM_BUILD} ..
ninja
cd ../tests/TemplateChecker
clang++ -emit-ast -c example.cpp
../../cmake-build-debug/tools/TemplateChecker/TemplateChecker astList.txt config.txt
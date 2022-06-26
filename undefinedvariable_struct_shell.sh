LLVM_BUILD="/home/cyrus/桌面/llvm-source-build/llvm-build"
cd ./cmake-build-debug
cmake -G Ninja -DLLVM_BUILD=${LLVM_BUILD} ..
ninja
cd ../tests/UndefinedVariableChecker/struct_test
clang++ -emit-ast -c struct.cpp
../../../cmake-build-debug/tools/UndefinedVariableChecker/UndefinedVariableChecker struct_astList.txt struct_config.txt
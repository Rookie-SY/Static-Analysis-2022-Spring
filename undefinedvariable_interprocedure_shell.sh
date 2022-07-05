LLVM_BUILD="/home/cyrus/桌面/llvm-source-build/llvm-build"
cd ./cmake-build-debug
cmake -G Ninja -DLLVM_BUILD=${LLVM_BUILD} ..
ninja
cd ../tests/UndefinedVariableChecker/interprocedure_test
clang++ -emit-ast -c interprocedure.cpp
../../../cmake-build-debug/tools/UndefinedVariableChecker/UndefinedVariableChecker interprocedure_astList.txt interprocedure_config.txt
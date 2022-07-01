LLVM_BUILD="/home/cyrus/桌面/llvm-source-build/llvm-build"
cd ./cmake-build-debug
cmake -G Ninja -DLLVM_BUILD=${LLVM_BUILD} ..
ninja
cd ../tests/UndefinedVariableChecker/PDG_test
clang++ -emit-ast -c FDT.cpp
../../../cmake-build-debug/tools/UndefinedVariableChecker/UndefinedVariableChecker FDT_astList.txt FDT_config.txt
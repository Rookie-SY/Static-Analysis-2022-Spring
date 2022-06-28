LLVM_BUILD="/home/cyrus/桌面/llvm-source-build/llvm-build"
cd ./cmake-build-debug
cmake -G Ninja -DLLVM_BUILD=${LLVM_BUILD} ..
ninja
cd ../tests/UndefinedVariableChecker/general_test
clang++ -emit-ast -c general.cpp
../../../cmake-build-debug/tools/UndefinedVariableChecker/UndefinedVariableChecker general_astList.txt general_config.txt
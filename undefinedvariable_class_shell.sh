LLVM_BUILD="/home/cyrus/桌面/llvm-source-build/llvm-build"
cd ./cmake-build-debug
cmake -G Ninja -DLLVM_BUILD=${LLVM_BUILD} ..
ninja
cd ../tests/UndefinedVariableChecker/class_test
clang++ -emit-ast -c class.cpp
../../../cmake-build-debug/tools/UndefinedVariableChecker/UndefinedVariableChecker class_astList.txt class_config.txt
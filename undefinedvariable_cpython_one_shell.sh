LLVM_BUILD="/home/cyrus/桌面/llvm-source-build/llvm-build"
cd ./cmake-build-debug
cmake -G Ninja -DLLVM_BUILD=${LLVM_BUILD} ..
ninja
cd ../tests/UndefinedVariableChecker/testsinglecase
../../../cmake-build-debug/tools/UndefinedVariableChecker/UndefinedVariableChecker astList.txt config.txt
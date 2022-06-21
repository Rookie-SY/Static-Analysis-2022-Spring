LLVM_BUILD="/home/cyrus/桌面/llvm-source-build/llvm-build"
cd ./cmake-build-debug
cmake -G Ninja -DLLVM_BUILD=${LLVM_BUILD} ..
ninja
cd ../tests/TemplateChecker
clang++ -emit-ast -c example.cpp
../../cmake-build-debug/tools/TemplateChecker/TemplateChecker astList.txt config.txt
#clang++ -emit-ast -c kill_gen.cpp
#../../cmake-build-debug/tools/UndefinedVariableChecker/UndefinedVariableChecker kill_gen_astList.txt kill_gen_config.txt
#clang++ -emit-ast -c array.cpp
#../../cmake-build-debug/tools/UndefinedVariableChecker/UndefinedVariableChecker array_astList.txt array_config.txt
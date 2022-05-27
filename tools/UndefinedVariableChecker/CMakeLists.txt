add_executable(UndefinedVariableChecker main.cpp)

# specify the libs that the target 'TemplateChecker' needs
# !!! DO NOT change the order of the libs !!!
# libs will be linked in this order, and changing the order could lead to link errors
target_link_libraries(UndefinedVariableChecker
  checkers
  framework
  ${CLANG_LIBS}
  ${LLVM_LIBS}
  ${OTHER_LIBS}
)

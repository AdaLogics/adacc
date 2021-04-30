#export SYMCC_REGULAR_LIBCXX=yes SYMCC_NO_SYMBOLIC_INPUT=yes 
BASE=${PWD}

rm -rf ./libcxx_native
rm -rf ./libcxx_native_build

mkdir ./libcxx_native
cd ./libcxx_native
cmake ../llvm_source/llvm \
		 -DLLVM_ENABLE_PROJECTS="clang" \
		 -DLLVM_TARGETS_TO_BUILD="X86" \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_INSTALL_PREFIX=${BASE}/libcxx_native_build 

#cmake -G Ninja ../llvm_source/llvm \
#		 -DLLVM_ENABLE_PROJECTS="clang;libc;libclc;libcxx;libcxxabi" \
#		 -DLLVM_TARGETS_TO_BUILD="X86" \
#		 -DCMAKE_BUILD_TYPE=Release \
#		 -DCMAKE_INSTALL_PREFIX=${BASE}/libcxx_native_build 
		 
#		 -DLLVM_DISTRIBUTION_COMPONENTS="libc;cxx;cxxabi;cxx-headers" \
#-DCMAKE_C_COMPILER=clang \
#		 -DCMAKE_CXX_COMPILER=clang++

#cmake -DCMAKE_C_COMPILER=clang \
#        -DCMAKE_CXX_COMPILER=clang++ \
#        -DLIBCXX_CXX_ABI=libcxxabi \
#		-DLLVM_TARGETS_TO_BUILD="X86" \
#		-DCMAKE_BUILD_TYPE=Release \
#		-DCMAKE_INSTALL_PREFIX=${BASE}/libcxx_native_build \
#        -DLIBCXX_CXX_ABI_INCLUDE_PATHS="/usr/include/c++/7.5.0/" \
#        ../llvm_source/libcxx

#        -DLIBCXX_CXX_ABI_INCLUDE_PATHS=path/to/separate/libcxxabi/include \
make
make check-cxx
#ninja 
#ninja check
#ninja distribution
#ninja install-distribution

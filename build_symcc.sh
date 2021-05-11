BASE=$PWD

install_packages_10() {
    echo "[+] installing packages"
	sudo apt-get update 
	sudo apt-get install -y \
		        python2 \
			cargo \
			clang-10 \
			cmake \
			g++ \
			git \
			llvm-10-dev \
			llvm-10-tools \
			ninja-build \
			python3-pip \
			zlib1g-dev 
	   
	pip3 install lit
    echo "[+] done installing packages"
}

install_packages_12() {
    echo "[+] installing packages"
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
    echo "Updated repo "
	sudo apt-get update 
	sudo apt-get install -y \
		        python2 \
			cargo \
			clang-12 \
			cmake \
			g++ \
			git \
			llvm-12-dev \
			llvm-12-tools \
			ninja-build \
			python3-pip \
			zlib1g-dev 
	pip3 install lit
    echo "[+] done installing packages"
}

get_z3() {
    # Build Z3
    cd ${BASE}
    git clone -b z3-4.8.7 https://github.com/Z3Prover/z3.git
    mkdir z3/build 
    cd z3/build 
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .. 
    ninja
    sudo ninja install

}

get_afl() {
    # Build AFL.
    cd ${BASE}
    git clone -b v2.56b https://github.com/google/AFL.git afl \
        && cd afl \
        && make

    # Download the LLVM sources already so that we don't need to get them again when
    # SymCC changes
    cd ${BASE}
    git clone -b llvmorg-10.0.1 --depth 1 https://github.com/llvm/llvm-project.git ./llvm_source
    echo "[+] finished installing deps"
}

get_qsym() {
	echo "[+] installing deps"
	#git clone https://github.com/eurecom-s3/symcc
    cd ${BASE}
	cd symcc/runtime/qsym_backend
    git clone https://github.com/AdaLogics/qsym
    cd qsym
    git checkout adalogics
    
	#git submodule init
	#git submodule update
}


install_symcc() {
	echo "[+] installing SymCC"
	# Build a version of SymCC with the simple backend to compile libc++
	mkdir symcc_build
	cd symcc_build
	cmake -G Ninja \
			-DQSYM_BACKEND=OFF \
			-DCMAKE_BUILD_TYPE=RelWithDebInfo -DZ3_TRUST_SYSTEM_VERSION=on \
			../symcc
    ninja check

	# Build SymCC with the Qsym backend
	cd ${BASE}
    rm -rf ./symcc_build_qsym
	mkdir symcc_build_qsym && cd symcc_build_qsym
	cmake -G Ninja \
			-DQSYM_BACKEND=ON  -DZ3_TRUST_SYSTEM_VERSION=ON  \
			-DCMAKE_BUILD_TYPE=RelWithDebInfo \
			../symcc 
    ninja check 
    cargo install --path ../symcc/util/symcc_fuzzing_helper
}

install_libcxx() {
    echo " installing libcxx"
    cd ${BASE}
	rm -rf ./libcxx_native-symbolic
	mkdir ./libcxx_native-symbolic
	cd ./libcxx_native-symbolic

	export SYMCC_REGULAR_LIBCXX=yes SYMCC_NO_SYMBOLIC_INPUT=yes
    
	#CFLAGS="-fsanitize-coverage=inline-8bit-counters" CXXFLAGS="-fsanitize-coverage=inline-8bit-counters" cmake ../llvm_source/llvm  \
    #export CFLAGS="${CFLAGS} -fsanitize-coverage=inline-8bit-counters"
    #export CXXFLAGS="${CXXFLAGS} -fsanitize-coverage=inline-8bit-counters"
	cmake ../llvm_source/llvm  \
			 -G Ninja \
			 -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi" \
			 -DLLVM_DISTRIBUTION_COMPONENTS="cxx;cxxabi;cxx-headers" \
			 -DLLVM_TARGETS_TO_BUILD="X86" \
			 -DCMAKE_BUILD_TYPE=Release \
			 -DCMAKE_INSTALL_PREFIX=${BASE}/libcxx_native_build \
			 -DCMAKE_C_COMPILER=${BASE}/symcc_build_qsym/symcc \
			 -DCMAKE_CXX_COMPILER=${BASE}/symcc_build_qsym/sym++ \
             -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON 

	ninja distribution
	ninja install-distribution

	echo "[+] done installing SymCC"
}

cleanup() {
    echo "[+] cleaning up"
    #sudo rm -rf ./afl
    #sudo rm -rf ./libcxx_symcc
    #sudo rm -rf ./llvm_source
    sudo rm -rf ./libcxx_symcc_install
    sudo rm -rf ./symcc_build
    sudo rm -rf ./symcc_build_qsym
    echo "[+] done cleaning up"
}

cd ${BASE}
echo "[+] Cleanup"
cleanup
echo "... Done"
cd ${BASE}
echo "[+] packages"
install_packages_12
cd ${BASE}
echo "[+] z3"
get_z3
cd ${BASE}
echo "[+] Afl"
get_afl
cd ${BASE}
echo "[+] qsym"
get_qsym
cd ${BASE}
echo "[+] symcc"
install_symcc
cd ${BASE}
echo "[+] libcxx"
install_libcxx

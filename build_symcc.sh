BASE=$PWD

install_packages() {
    echo "[+] installing packages"
	sudo apt-get update 
	sudo apt-get install -y \
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

get_deps() {
	echo "[+] installing deps"
	#git clone https://github.com/eurecom-s3/symcc
	cd symcc/runtime/qsym_backend
    git clone https://github.com/AdaLogics/qsym
    cd qsym
    git checkout adalogics
    
	#git submodule init
	#git submodule update

    # Build Z3
    cd ${BASE}
    git clone -b z3-4.8.6 https://github.com/Z3Prover/z3.git
    mkdir z3/build 
    cd z3/build 
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .. 
    ninja
    sudo ninja install

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

	# Build libc++ with SymCC using the simple backend
    #rm -rf libcxx_symcc
    #rm -rf libcxx_symcc_install
	#cd ${BASE} && mkdir libcxx_symcc && cd libcxx_symcc
	#export SYMCC_REGULAR_LIBCXX=yes SYMCC_NO_SYMBOLIC_INPUT=yes 
	#mkdir ./libcxx_symcc_build 
    #cd ./libcxx_symcc_build 
	#cmake -G Ninja ${BASE}/llvm_source/llvm \
#			 -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi" \
#			 -DLLVM_TARGETS_TO_BUILD="X86" \
#			 -DLLVM_DISTRIBUTION_COMPONENTS="cxx;cxxabi;cxx-headers" \
#			 -DCMAKE_BUILD_TYPE=Release \
#			 -DCMAKE_INSTALL_PREFIX=${BASE}/libcxx_symcc_install \
#			 -DCMAKE_C_COMPILER=${BASE}/symcc_build/symcc \
#			 -DCMAKE_CXX_COMPILER=${BASE}/symcc_build/sym++ 
#    ninja distribution 
#    ninja install-distribution
    echo " installing libcxx"
	rm -rf ./libcxx_native-symbolic
	mkdir ./libcxx_native-symbolic
	cd ./libcxx_native-symbolic

	export SYMCC_REGULAR_LIBCXX=yes SYMCC_NO_SYMBOLIC_INPUT=yes
	cmake ../llvm-project/llvm  \
			 -G Ninja \
			 -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi" \
			 -DLLVM_DISTRIBUTION_COMPONENTS="cxx;cxxabi;cxx-headers" \
			 -DLLVM_TARGETS_TO_BUILD="X86" \
			 -DCMAKE_BUILD_TYPE=Release \
			 -DCMAKE_INSTALL_PREFIX=${BASE}/libcxx_native_build \
			 -DCMAKE_C_COMPILER=${BASE}/symcc_build/symcc \
			 -DCMAKE_CXX_COMPILER=${BASE}/symcc_build/sym++
	ninja distribution
	ninja install-distribution

	echo "[+] done installing SymCC"
}

cleanup() {
    echo "[+] cleaning up"
    sudo rm -rf ./afl
    sudo rm -rf ./libcxx_symcc
    sudo rm -rf ./libcxx_symcc_install
    sudo rm -rf ./llvm_source
    sudo rm -rf ./symcc_build
    sudo rm -rf ./symcc_build_qsym
    echo "[+] done cleaning up"
}

cleanup
#install_packages
get_deps
install_symcc

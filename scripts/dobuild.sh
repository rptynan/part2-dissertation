#!/usr/local/bin/bash

function print_usage(){
	echo "Usage:"
	echo "-g, --good-kernel: Copy kernel.good over current kernel as backup"
	echo "-b, --build <compiler>: Do a build with that compiler"
	echo "-r, --install-reboot: Install kernel and reboot"
	echo ""
	echo "-d, --debug: Turn on debug flags for build"
	echo "--remake-everything: Disable KERNFAST and build kernel from scratch"
}

GOOD_KERNEL=false
COMPILER=""
INSTALL_REBOOT=false

KERNFAST="-DKERNFAST"
DEBUG_VALUE=0
while (( $# > 0 )); do
	key="$1"
	case $key in
		-g|--good-kernel)
			GOOD_KERNEL=true
			;;
		-r|--install-reboot)
			INSTALL_REBOOT=true
			;;
		-b|--build)
			COMPILER="$2"
			shift
			;;
		--remake-everything)
			KERNFAST=""
			;;
		-d|--debug)
			DEBUG_VALUE=1
			;;
		-h|--help|*)
			print_usage
			exit
			;;
	esac
	shift
done
DEBUG_FLAGS="-DDEBUG_CIL_INLINES=$DEBUG_VALUE -DDEBUG_STUBS=$DEBUG_VALUE"
[[ $DEBUG_VALUE == 0 ]] && DEBUG_FLAGS+=" -DNDEBUG"

if $GOOD_KERNEL; then
	if [ $(md5 -q /boot/kernel/kernel) == $(md5 -q /boot/kernel.good/kernel) ]; then
		echo "/boot/kernel is same as /boot/kernel.good, not copying"
	else
		echo "Copying /boot/kernel.good over /boot/kernel"
		rm -rf /boot/kernel
		cp -r /boot/kernel.good/ /boot/kernel/
	fi
fi

if [[ ! -z $COMPILER ]]; then
	echo "Doing build with compiler $COMPILER"
	echo "Output is in ~/errorlog and ~/log"
	case $COMPILER in
		"gcc")
			CC="/usr/local/bin/gcc -gdwarf-3 -gdwarf-2 "
			CC+="-D_MM_MALLOC_H_INCLUDED $DEBUG_FLAGS"
			;;
		"crunchcc")
			CC="/usr/local/src/libcrunch/frontend/c/bin/crunchcc "
			CC+="-gdwarf-3 -gdwarf-2 -D_MM_MALLOC_H_INCLUDED "
			CC+="--useLogicalOperators -DKTR $DEBUG_FLAGS"
			;;
		*)
			echo "Compiler '$COMPILER' not recognised"
			exit
			;;
	esac
	CC=$CC \
	COPTFLAGS="-O -pipe" \
	COMPILER_TYPE="gcc" \
	CPPFLAGS="" CXXFLAGS="" LDFLAGS="--wrap malloc --wrap free" \
	WITHOUT_FORMAT_EXTENSIONS=no \
	/usr/bin/time \
	make buildkernel $KERNFAST MODULES_OVERRIDE= KERNCONF=CRUNCHED \
	1>  ~/log 2> ~/errorlog
	if [[ $? != 0 ]]; then
		echo "Build failed, logs in ~, exiting"
		exit
	fi
fi

if $INSTALL_REBOOT; then
	[ ! $GOOD_KERNEL ] && \
		echo -n "Warning, haven't copied good kernel! " && \
		echo "Might not be able to boot if kernel is bad!"
	echo "Installing"
	make installkernel KERNCONF=CRUNCHED 1> ~/loginstall 2> ~/errorloginstall
	if [[ $? != 0 ]]; then
		echo "Install failed, logs in ~, exiting"
		exit
	fi
	echo "Rebooting"
	reboot
fi

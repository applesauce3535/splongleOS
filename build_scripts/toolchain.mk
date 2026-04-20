TOOLCHAIN_PREFIX = $(abspath toolchain/$(TARGET))
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)

toolchain: toolchain_binutils toolchain_gcc

BINUTILS_SRC = toolchain/binutils-2.46
BINUTILS_BUILD = toolchain/binutils-build
GCC_SRC = toolchain/gcc-15.2.0
GCC_BUILD = toolchain/gcc-build

toolchain_binutils: toolchain/binutils-$(BINUTILS_VERSION).tar.xz
	cd toolchain && tar -xf binutils-$(BINUTILS_VERSION).tar.xz
	mkdir $(BINUTILS_BUILD)
	cd $(BINUTILS_BUILD) && CFLAGS= ASMFLAGS= CC= CXX= ASM= LINKFLAGS= LIBS= ../binutils-$(BINUTILS_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)"	\
		--target=$(TARGET)				\
		--with-sysroot					\
		--disable-nls					\
		--disable-werror
	$(MAKE) -j 8 -C $(BINUTILS_BUILD)
	$(MAKE) -j 8 -C $(BINUTILS_BUILD) install

toolchain/binutils-$(BINUTILS_VERSION).tar.xz:
	mkdir -p toolchain
	cd toolchain && wget $(BINUTILS_URL)

toolchain_gcc: toolchain_binutils toolchain/gcc-$(GCC_VERSION).tar.gz
	cd toolchain && tar -xf gcc-$(GCC_VERSION).tar.gz
	mkdir $(GCC_BUILD)
	cd $(GCC_BUILD) && CFLAGS= ASMFLAGS= CC= CXX= ASM= LINKFLAGS= LIBS= ../gcc-$(GCC_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)"	\
		--target=$(TARGET)				\
		--disable-nls					\
		--enable-languages=c,c++		\
		--without-headers
	$(MAKE) -j 8 -C $(GCC_BUILD) all-gcc all-target-libgcc
	$(MAKE) -j 8 -C $(GCC_BUILD) install-gcc install-target-libgcc
	
toolchain/gcc-$(GCC_VERSION).tar.gz:
	mkdir -p toolchain
	cd toolchain && wget $(GCC_URL)

#
# clean
#
clean-toolchain:
	rm -rf $(GCC_BUILD) $(GCC_SRC) $(BINUTILS_BUILD) $(BINUTILS_SRC)

clean-toolchain-all:
	rm -rf toolchain/*

.PHONY: toolchain toolchain_binutils toolchain_gcc

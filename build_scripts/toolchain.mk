TOOLCHAIN_PREFIX = $(abspath toolchain/$(TARGET))
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)

toolchain: toolchain_binutils toolchain_gcc

BINUTILS_SRC = toolchain/binutils-2.44
BINUTILS_BUILD = toolchain/binutils-build
GCC_SRC = toolchain/gcc-15.1.0
GCC_BUILD = toolchain/gcc-build

toolchain_binutils: $(TOOLCHAIN_PREFIX)/bin/i686-elf-ld

$(TOOLCHAIN_PREFIX)/bin/i686-elf-ld: $(BINUTILS_SRC).tar.xz
	cd toolchain && tar -xf binutils-$(BINUTILS_VERSION).tar.xz
	mkdir $(BINUTILS_BUILD)
	cd $(BINUTILS_BUILD) && CFLAGS= ASMFLAGS= CC= CXX= LD= ASM= LINKFLAGS= LIBS= ../binutils-$(BINUTILS_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)"	\
		--target=$(TARGET)	
TOOLCHAIN_PREFIX = $(abspath toolchain/$(TARGET))
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)

toolchain: toolchain_binutils toolchain_gcc

BINUTILS_SRC = toolchain/binutils-$(BINUTILS_VERSION)
BINUTILS_BUILD = toolchain/binutils-build-$(BINUTILS_VERSION)

toolchain_binutils: $(TOOLCHAIN_PREFIX)/bin/i686-elf-ld

$(TOOLCHAIN_PREFIX)/bin/i686-elf-ld: $(BINUTTOOLCHAIN_PREFIX = $(abspath toolchain/$(TARGET))
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)

toolchain: toolchain_binutils toolchain_gcc

BINUTILS_SRC = toolchain/binutils-2.44
BINUTILS_BUILD = toolchain/binutils-build
GCC_SRC = toolchain/gcc-15.1.0
GCC_BUILD = toolchain/gcc-build

toolchain_binutils: toolchain/binutils-$(BINUTILS_VERSION).tar.xz
	cd toolchain && tar -xf binutils-$(BINUTILS_VERSION).tar.xz
	mkdir $(BINUTILS_BUILD)
	cd $(BINUTILS_BUILD) && ../binutils-$(BINUTILS_VERSION)/configure \
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
	cd $(GCC_BUILD) && ../gcc-$(GCC_VERSION)/configure \
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

.PHONY: toolchain toolchain_binutils toolchain_gccILS_SRC).tar.xz
	cd toolchain && tar -xf binutils-$(BINUTILS_VERSION).tar.xz
	mkdir $(BINUTILS_BUILD)
	cd $(BINUTILS_BUILD) && CFLAGS= ASMFLAGS= CC= CXX= LD= ASM= LINKFLAGS= LIBS= ../binutils-$(BINUTILS_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)"	\
		--target=$(TARGET)				\
		--with-sysroot					\
		--disable-nls					\
		--disable-werror
	$(MAKE) -j8 -C $(BINUTILS_BUILD)
	$(MAKE) -C $(BINUTILS_BUILD) install

$(BINUTILS_SRC).tar.xz:
	mkdir -p toolchain 
	cd toolchain && wget $(BINUTILS_URL)


GCC_SRC = toolchain/gcc-$(GCC_VERSION)
GCC_BUILD = toolchain/gcc-build-$(GCC_VERSION)

toolchain_gcc: $(TOOLCHAIN_PREFIX)/bin/i686-elf-gcc

$(TOOLCHAIN_PREFIX)/bin/i686-elf-gcc: $(TOOLCHAIN_PREFIX)/bin/i686-elf-ld $(GCC_SRC).tar.xz
	cd toolchain && tar -xf gcc-$(GCC_VERSION).tar.xz
	mkdir $(GCC_BUILD)
	cd $(GCC_BUILD) && CFLAGS= ASMFLAGS= CC= CXX= LD= ASM= LINKFLAGS= LIBS= ../gcc-$(GCC_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)" 	\
		--target=$(TARGET)				\
		--disable-nls					\
		--enable-languages=c,c++		\
		--without-headers
	$(MAKE) -j8 -C $(GCC_BUILD) all-gcc all-target-libgcc
	$(MAKE) -C $(GCC_BUILD) install-gcc install-target-libgcc
	
$(GCC_SRC).tar.xz:
	mkdir -p toolchain
	cd toolchain && wget $(GCC_URL)

#
# Clean
#
clean-toolchain:
	rm -rf $(GCC_BUILD) $(GCC_SRC) $(BINUTILS_BUILD) $(BINUTILS_SRC)

clean-toolchain-all:
	rm -rf toolchain/*

.PHONY:  toolchain toolchain_binutils toolchain_gcc clean-toolchain clean-toolchain-all			\
		--with-sysroot					\
		--disable-nls					\
		--disable-werror
	$(MAKE) -j 8 -C $(BINUTILS_BUILD)
	$(MAKE) -j 8 -C $(BINUTILS_BUILD) install

$(BINUTILS_SRC).tar.xz:
	mkdir -p toolchain
	cd toolchain && wget $(BINUTILS_URL)

toolchain_gcc: $(TOOLCHAIN_PREFIX)/bin/i686-elf-gcc

$(TOOLCHAIN_PREFIX)/bin/i686-elf-gcc: toolchain_binutils $(GCC_SRC).tar.gz
	cd toolchain && tar -xf gcc-$(GCC_VERSION).tar.gz
	mkdir $(GCC_BUILD)
	cd $(GCC_BUILD) && CFLAGS= ASMFLAGS= CC= CXX= LD= ASM= LINKFLAGS= LIBS= ../gcc-$(GCC_VERSION)/configure \
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

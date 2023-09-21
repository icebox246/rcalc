CC=clang
SOURCES+=src/main.c
SOURCES+=src/imgui.c

ifndef ANDROID

LIBS+=`pkg-config --libs raylib`
CFLAGS+=`pkg-config --cflags raylib` -ggdb

else
ANDROID_TOOLCHAIN=$(ANDROID_NDK)/toolchains/llvm/prebuilt/linux-x86_64

CC=${ANDROID_TOOLCHAIN}/bin/aarch64-linux-android29-clang
AR=${ANDROID_TOOLCHAIN}/bin/aarch64-linux-android29-ar

LIBS+=-lm -lc -llog -lraylib -landroid -lEGL -lGLESv2 -lOpenSLES -ldl -latomic

LDFLAGS+=-Wl,-soname,librcalc.so -Wl,--exclude-libs,libatomic.a -no-canonical-prefixes
LDFLAGS+=-Wl,--build-id -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--warn-shared-textrel -Wl,--fatal-warnings
LDFLAGS+=-u ANativeActivity_onCreate

CFLAGS+=-Iraylib -Lraylib
CFLAGS+=-std=c99 -march=armv8-a -mfix-cortex-a53-835769
CFLAGS+=-ffunction-sections -funwind-tables -fstack-protector-strong -fPIC
CFLAGS+=-Wall -Wa,--noexecstack -Wformat -Werror=format-security -no-canonical-prefixes
CFLAGS+=-D__ANDROID__ -DPLATFORM_ANDROID

SOURCES+=${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c

endif

rcalc: check ${SOURCES}
ifndef ANDROID
	${CC} ${SOURCES} ${LIBS} ${CFLAGS} -o $@
else
	${CC} ${SOURCES} ${LIBS} ${LDFLAGS} ${CFLAGS} -shared -o lib$@.so
	[ -d android-shim/app/src/main/jniLibs/arm64-v8a/ ] || mkdir -p android-shim/app/src/main/jniLibs/arm64-v8a/
	cp lib$@.so android-shim/app/src/main/jniLibs/arm64-v8a/
	cd android-shim && ./gradlew build
endif

install: rcalc
	cd android-shim && ./gradlew installDebug

check:
ifdef ANDROID
ifndef JAVA_HOME
	@echo JAVA_HOME must be provided
	@exit 1
endif
ifndef ANDROID_NDK
	@echo ANDROID_NDK must be provided
	@exit 1
endif
endif

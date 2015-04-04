HEADERS 	= $(find . -iname "*.h")

BIN=bin
SRC=src

OPTIONS         = -std=c11
OPTIONS        += -I$(SRC)/headers/
OPTIONS        += -Wall -Wextra -pedantic -Werror

LINUX_OPTIONS	= $(OPTIONS)
TEMP_LINUX	= tmp/linux

URL_FILE_BASE   = static/

LINUX_TARGET	= $(BIN)/linux
LINUX_GEN_DEST = FilesAsCArrays

LINUX_GENERATED = $(shell find ${TEMP_LINUX}/${LINUX_GEN_DEST} -iname "*.o")

LINUX_FILES  = $(TEMP_LINUX)/main.o
LINUX_FILES += $(TEMP_LINUX)/server/server.o
LINUX_FILES += $(TEMP_LINUX)/server/binaryUtils.o
LINUX_FILES += $(TEMP_LINUX)/server/protocol/arp.o
LINUX_FILES += $(TEMP_LINUX)/server/protocol/ip.o
LINUX_FILES += $(TEMP_LINUX)/enc28j60emu.o
LINUX_FILES += $(TEMP_LINUX)/files.g1.o

all: linux

linux: $(LINUX_TARGET)

$(LINUX_TARGET): $(LINUX_FILES)
	gcc $(LINUX_OPTIONS) $(LINUX_FILES) $(LINUX_GENERATED) -o $(LINUX_TARGET)

$(TEMP_LINUX)/%.o: $(SRC)/%.c $(HEADERS)
	@mkdir -p "$(shell dirname "$@")"
	gcc $(LINUX_OPTIONS) -c $< -o $@

$(TEMP_LINUX)/%.g1.o: $(TEMP_LINUX)/%.g1.c $(TEMP_LINUX)/%.g1.h
	gcc $(LINUX_OPTIONS) -Wno-overlength-strings -c $< -o $@

$(TEMP_LINUX)/%.g1.c $(TEMP_LINUX)/%.g1.h: 2cstr
	./genCode.sh $(shell basename $@ | head -c -6) $(shell basename $@ | head -c -6) $(URL_FILE_BASE) $(TEMP_LINUX) $(LINUX_GEN_DEST) $(shell basename $@ | head -c -6)

2cstr:
	gcc -std=c11 $(SRC)/2cstr.c -o 2cstr

clean:
	rm -rf $(TEMP_LINUX) $(LINUX_TARGET) *~ 2cstr


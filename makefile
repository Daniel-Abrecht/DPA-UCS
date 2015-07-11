#########################
####    VARIABLES    ####
#########################

###################
#     GENERAL     #
###################

HEADERS 	= $(find . -iname "*.h")

BIN=bin
SRC=src

SYSTEM_CC=gcc
LINUX_CC=gcc
AVR_CC=avr-gcc
AVR_MCU=attiny8
AVR_NET_DRIVER=dummy

OPTIONS         = -std=c11 -Os
OPTIONS        += -I$(SRC)/headers/
OPTIONS        += -Wall -Wextra -pedantic -Werror
OPTIONS        += -s -ffunction-sections -fdata-sections -Wl,-gc-sections

GEN_DEST = FilesAsCArrays
URL_FILE_BASE   = static/


FILES  = main.o
FILES += server/server.o
FILES += server/stream.o
FILES += server/mempool.o
FILES += server/checksum.o
FILES += server/binaryUtils.o
FILES += server/protocol/arp.o
FILES += server/protocol/tcp.o
FILES += server/protocol/icmp.o
FILES += server/protocol/ip.o
FILES += server/protocol/IPv4.o
FILES += server/protocol/layer3.o
FILES += server/protocol/ip_stack.o
FILES += server/protocol/address.o
FILES += server/protocol/tcp_ip_stack_memory.o
FILES += server/services/http.o
FILES += files.g1.o


###################
#      LINUX      #
###################

LINUX_OPTIONS	= $(OPTIONS)
TEMP_LINUX	= tmp/linux
LINUX_TARGET	= $(BIN)/linux
LINUX_GENERATED = $(shell find ${TEMP_LINUX}/${GEN_DEST} -iname "*.o")

LINUX_FILES_TMP  = $(FILES)
LINUX_FILES_TMP += server/drivers/eth/linux.o


LINUX_FILES = $(shell \
  for file in ${LINUX_FILES_TMP}; \
  do \
    echo "${TEMP_LINUX}/$$file"; \
  done; \
)


###################
#      LINUX      #
###################

AVR_OPTIONS	= $(OPTIONS)
TEMP_AVR	= tmp/avr_$(AVR_MCU)
AVR_TARGET	= $(BIN)/avr_$(AVR_MCU)
AVR_GENERATED   = $(shell find ${TEMP_LINUX}/${GEN_DEST} -iname "*.o")

AVR_FILES_TMP  = $(FILES)
AVR_FILES_TMP += server/drivers/$(AVR_NET_DRIVER).o


AVR_FILES = $(shell \
  for file in ${AVR_FILES_TMP}; \
  do \
    echo "${TEMP_AVR}/$$file"; \
  done; \
)


#########################
####     TARGETS     ####
#########################

###################
#     GENERAL     #
###################

all: bin linux avr

linux: bin $(LINUX_TARGET)

avr: bin $(AVR_TARGET)

bin:
	@mkdir $(BIN)

2cstr:
	$(SYSTEM_CC) -std=c11 $(SRC)/2cstr.c -o 2cstr

clean: clean_linux clean_avr
	rm -f 2cstr


###################
#      LINUX      #
###################

$(LINUX_TARGET): $(LINUX_FILES)
	$(LINUX_CC) $(LINUX_OPTIONS) $(LINUX_FILES) $(LINUX_GENERATED) -o $(LINUX_TARGET)

$(TEMP_LINUX)/%.o: $(SRC)/%.c $(HEADERS)
	@mkdir -p "$(shell dirname "$@")"
	$(LINUX_CC) $(LINUX_OPTIONS) -c $< -o $@

$(TEMP_LINUX)/%.g1.o: $(TEMP_LINUX)/%.g1.c $(TEMP_LINUX)/%.g1.h
	$(LINUX_CC) $(LINUX_OPTIONS) -Wno-overlength-strings -c $< -o $@

$(TEMP_LINUX)/%.g1.c $(TEMP_LINUX)/%.g1.h: 2cstr
	./genCode.sh $(shell basename $@ | head -c -6) $(shell basename $@ | head -c -6) $(URL_FILE_BASE) $(TEMP_LINUX) $(GEN_DEST) $(shell basename $@ | head -c -6)

clean_linux:
	rm -rf $(TEMP_LINUX) $(LINUX_TARGET)


###################
#       AVR       #
###################

$(AVR_TARGET): $(AVR_FILES)
	$(AVR_CC) $(AVR_OPTIONS) $(AVR_FILES) $(AVR_GENERATED) -o $(AVR_TARGET)

$(TEMP_AVR)/%.o: $(SRC)/%.c $(HEADERS)
	@mkdir -p "$(shell dirname "$@")"
	$(AVR_CC) $(AVR_OPTIONS) -c $< -o $@

$(TEMP_AVR)/%.g1.o: $(TEMP_AVR)/%.g1.c $(TEMP_AVR)/%.g1.h
	$(AVR_CC) $(AVR_OPTIONS) -Wno-overlength-strings -c $< -o $@

$(TEMP_AVR)/%.g1.c $(TEMP_AVR)/%.g1.h: 2cstr
	./genCode.sh $(shell basename $@ | head -c -6) $(shell basename $@ | head -c -6) $(URL_FILE_BASE) $(TEMP_AVR) $(GEN_DEST) $(shell basename $@ | head -c -6)

clean_avr:
	rm -rf $(TEMP_AVR) $(AVR_TARGET)


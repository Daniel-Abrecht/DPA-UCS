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
AVR_MCU=atmega16
AVR_NET_DRIVER=dummy
AVR_F_CPU = 3686400UL

OPTIONS        += -std=c11 #--short-enums
OPTIONS        += -I$(SRC)/headers/
OPTIONS        += -Wall -Wextra -pedantic -Werror
ifdef DEBUG
OPTIONS        += -Og -g -DDEBUG
else
OPTIONS        += -Os #-flto
OPTIONS        += -ffast-math
OPTIONS        += -s -ffunction-sections -fdata-sections
endif

GEN_DEST = FilesAsCArrays
URL_FILE_BASE   = static/

USE_IPv4=true
#USE_IPv6=true

OPTIONAL_FILES  += server/protocol/icmp.o
OPTIONAL_FILES  += server/adelay.o
OPTIONAL_FILES  += server/services/http.o
ifndef NO_TCP
OPTIONAL_FILES  += server/protocol/tcp.o server/protocol/tcp_stack.o server/protocol/tcp_retransmission_cache.o
DPAUCS_INIT     += X(DPAUCS_tcpInit)
DPAUCS_SHUTDOWN += X(DPAUCS_tcpShutdown)
OPTIONS += -DUSE_TCP
endif

ifdef USE_IPv4
OPTIONAL_FILES += server/protocol/IPv4.o
OPTIONS        += -DUSE_IPv4
endif
ifdef USE_IPv6
OPTIONS        += -DUSE_IPv6
endif

FILES += main.o
FILES += server/server.o
FILES += server/utils.o
FILES += server/stream.o
FILES += server/mempool.o
FILES += server/checksum.o
FILES += server/ressource.o
FILES += server/binaryUtils.o
FILES += server/protocol/arp.o
FILES += server/protocol/ip.o
FILES += server/protocol/layer3.o
FILES += server/protocol/ip_stack.o
FILES += server/protocol/address.o
FILES += server/protocol/tcp_ip_stack_memory.o
FILES += files.g1.o
FILES += $(OPTIONAL_FILES)


###################
#      LINUX      #
###################

LINUX_OPTIONS	= $(OPTIONS)
TEMP_LINUX	= tmp/linux
LINUX_TARGET	= $(BIN)/linux
LINUX_GENERATED = $(shell find ${TEMP_LINUX}/${GEN_DEST} -iname "*.o")

LINUX_FILES_TMP  = $(FILES)
LINUX_FILES_TMP += server/drivers/eth/linux.o
LINUX_FILES_TMP += server/drivers/adelay_clock.o

ifdef DPAUCS_INIT
LINUX_DPAUCS_INIT += $(DPAUCS_INIT)
endif
ifdef LINUX_DPAUCS_INIT
LINUX_OPTIONS     += -DDPAUCS_INIT='$(LINUX_DPAUCS_INIT)'
endif

ifdef DPAUCS_SHUTDOWN
LINUX_DPAUCS_SHUTDOWN += $(DPAUCS_SHUTDOWN)
endif
ifdef LINUX_DPAUCS_SHUTDOWN
LINUX_OPTIONS  += -DDPAUCS_SHUTDOWN='$(LINUX_DPAUCS_SHUTDOWN)'
endif

LINUX_FILES = $(shell \
  for file in ${LINUX_FILES_TMP}; \
  do \
    echo "${TEMP_LINUX}/$$file"; \
  done; \
)


###################
#       AVR       #
###################

AVR_OPTIONS	= $(OPTIONS) -DF_CPU=$(AVR_F_CPU) -mmcu=$(AVR_MCU)
TEMP_AVR	= tmp/avr_$(AVR_MCU)
AVR_TARGET	= $(BIN)/avr_$(AVR_MCU)
AVR_GENERATED   = $(shell find ${TEMP_LINUX}/${GEN_DEST} -iname "*.o")

AVR_FILES_TMP  = $(FILES)
AVR_FILES_TMP += server/drivers/$(AVR_NET_DRIVER).o

ifndef NO_ADELAY_DRIVER
AVR_FILES_TMP   += server/drivers/adelay_timer.o
AVR_DPAUCS_INIT += X(adelay_timer_init)
endif

ifdef DPAUCS_INIT
AVR_DPAUCS_INIT += $(DPAUCS_INIT)
endif
ifdef AVR_DPAUCS_INIT
AVR_OPTIONS += -DDPAUCS_INIT='$(AVR_DPAUCS_INIT)'
endif

ifdef DPAUCS_SHUTDOWN
AVR_DPAUCS_SHUTDOWN = $(DPAUCS_SHUTDOWN)
endif
ifdef AVR_DPAUCS_SHUTDOWN
AVR_OPTIONS += -DDPAUCS_SHUTDOWN='$(AVR_DPAUCS_SHUTDOWN)'
endif

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
	# First pass, won't remove unused symbols because it will supress undefined reference errors
	# in unused symbols and they are used for dependendy checking
	$(LINUX_CC) $(LINUX_OPTIONS) $(LINUX_FILES) $(LINUX_GENERATED) -o $(LINUX_TARGET)_tmp
	rm $(LINUX_TARGET)_tmp
	# Second pass, there isn't any undefined reference, remove all unused symbols
	$(LINUX_CC) $(LINUX_OPTIONS) -Wl,-gc-sections $(LINUX_FILES) $(LINUX_GENERATED) -o $(LINUX_TARGET)

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
	$(AVR_CC) $(AVR_OPTIONS) -Wl,-gc-sections $(AVR_FILES) $(AVR_GENERATED) -o $(AVR_TARGET)

$(TEMP_AVR)/%.o: $(SRC)/%.c $(HEADERS)
	@mkdir -p "$(shell dirname "$@")"
	$(AVR_CC) $(AVR_OPTIONS) -c $< -o $@

$(TEMP_AVR)/%.g1.o: $(TEMP_AVR)/%.g1.c $(TEMP_AVR)/%.g1.h
	$(AVR_CC) $(AVR_OPTIONS) -Wno-overlength-strings -c $< -o $@

$(TEMP_AVR)/%.g1.c $(TEMP_AVR)/%.g1.h: 2cstr
	./genCode.sh $(shell basename $@ | head -c -6) $(shell basename $@ | head -c -6) $(URL_FILE_BASE) $(TEMP_AVR) $(GEN_DEST) $(shell basename $@ | head -c -6)

clean_avr:
	rm -rf $(TEMP_AVR) $(AVR_TARGET)


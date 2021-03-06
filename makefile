#########################
####    VARIABLES    ####
#########################

###################
#     GENERAL     #
###################

HEADERS = $(find . -iname "*.h")

BIN=bin
SRC=src
LIB=lib

SYSTEM_CC=gcc
LINUX_CC=gcc
LINUX_AR=gcc-ar
AVR_CC=avr-gcc
AVR_AR=avr-ar
AVR_MCU=atmega1284
AVR_F_CPU = 16000000UL

LINUX_OPTIONS    += -std=c11
# gnu11 Needed for __flash support
AVR_OPTIONS      += -std=gnu11
OPTIONS          += -I$(SRC)/headers/ -L$(BIN)
OPTIONS          += -Wall -Wextra -pedantic -Werror
DEBUG_OPTIONS    += -Og -g -DDEBUG
SANITIZE_OPTIONS += -fprofile-arcs -ftest-coverage -static-libasan -fsanitize=undefined -fsanitize=address -fstack-protector-all
PROD_OPTIONS     += -Os -ffast-math -ffunction-sections -fdata-sections

#OPTIONS         += --short-enums

LINUX_LIBS=

ifndef_any_of = $(filter undefined,$(foreach v,$(1),$(origin $(v))))
ifdef_any_of = $(filter-out undefined,$(foreach v,$(1),$(origin $(v))))

ifneq ($(call ifdef_any_of,DEBUG),)
TMP_OPTIONS += $(DEBUG_OPTIONS)
else
TMP_OPTIONS += $(PROD_OPTIONS)
endif

ifneq ($(call ifdef_any_of,DEBUG SANITIZE),)
TMP_OPTIONS += $(SANITIZE_OPTIONS)
endif

ifdef NO_LOGGING
TMP_OPTIONS += -DNO_LOGGING
endif

PROJECT_DIR=$(PWD)

GEN_DEST = FilesAsCArrays
URL_FILE_BASE   = 

OPTIONAL_FILES += example/websocket/echo.o
OPTIONAL_FILES += server/adelay.o
OPTIONAL_FILES += server/ressource.o
OPTIONAL_FILES += server/services/dhcp_client.o
OPTIONAL_FILES += server/services/http.o
OPTIONAL_FILES += server/services/websocket.o
OPTIONAL_FILES += server/services/icmp.o
OPTIONAL_FILES += server/protocol/udp.o
OPTIONAL_FILES += server/protocol/tcp/tcp.o
OPTIONAL_FILES += server/protocol/tcp/tcp_stack.o
OPTIONAL_FILES += server/protocol/tcp/tcp_retransmission_cache.o
OPTIONAL_FILES += server/protocol/IPv4.o
OPTIONAL_FILES += utils/logger.o

OPTIONAL_FILES += files.g1.o
OPTIONAL_FILES += server/files.o

MAIN_FILE = main.o

FILES += server/server.o
FILES += utils/utils.o
FILES += utils/crypto/sha1.o
FILES += utils/encoding/base64.o
FILES += utils/ringbuffer.o
FILES += utils/stream.o
FILES += utils/mempool.o
FILES += server/checksum.o
FILES += server/protocol/arp.o
FILES += server/protocol/layer3.o
FILES += server/protocol/ip_stack.o
FILES += server/protocol/address.o
FILES += server/protocol/fragment_memory.o
FILES += $(OPTIONAL_FILES)

###################
#      LINUX      #
###################

TMP_LINUX_OPTIONS += $(OPTIONS)
LINUX_OPTIONS   += $(TMP_LINUX_OPTIONS) $(TMP_OPTIONS)
TEST_OPTIONS    += -lgcov $(TMP_LINUX_OPTIONS) $(DEBUG_OPTIONS) $(SANITIZE_OPTIONS)
TEMP_LINUX	 = tmp/linux
LINUX_NAME	 = dpaucs-linux
LINUX_TARGET	 = $(BIN)/$(LINUX_NAME)
LINUX_LIBRARY    = $(BIN)/lib$(LINUX_NAME).a
LINUX_GENERATED  = #$(shell find ${TEMP_LINUX}/${GEN_DEST} -iname "*.o")

LINUX_OPTIONS  += -I${TEMP_LINUX}

LINUX_FILES_TMP  = $(FILES)
LINUX_FILES_TMP += server/driver/linux/ethernet.o
LINUX_FILES_TMP += server/driver/linux/adelay.o

LINUX_FILES = $(shell \
  for file in ${LINUX_FILES_TMP}; \
  do \
    echo "${TEMP_LINUX}/$$file"; \
  done; \
)

CRITERION_SRC=$(LIB)/Criterion
TESTS=$(shell \
  for file in "$(SRC)"/test/*.c; do \
    echo "test-$$(basename "$$file" .c)"; \
  done \
)

###################
#       AVR       #
###################

AVR_OPTIONS	+= $(OPTIONS) $(TMP_OPTIONS) -Waddr-space-convert -DF_CPU=$(AVR_F_CPU) -mmcu=$(AVR_MCU)
TEMP_AVR	 = tmp/avr_$(AVR_MCU)
AVR_NAME	 = dpaucs-$(AVR_MCU)
AVR_TARGET	 = $(BIN)/$(AVR_NAME)
AVR_LIBRARY	 = $(BIN)/lib$(AVR_NAME).a
AVR_GENERATED    = #$(shell find ${TEMP_LINUX}/${GEN_DEST} -iname "*.o")

AVR_OPTIONS  += -I${TEMP_AVR}

AVR_FILES_TMP  = $(FILES)
AVR_FILES_TMP += config/board/avr-net-io.o
AVR_FILES_TMP += server/driver/generic/enc28j60.o
AVR_FILES_TMP += server/driver/atmega1284/spi.o
AVR_FILES_TMP += server/driver/atmega1284/io_pin.o
AVR_FILES_TMP += server/driver/atmega1284/stdio_to_uart.o

ifndef NO_ADELAY_DRIVER
AVR_FILES_TMP   += server/driver/atmega1284/adelay.o
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

linux: bin $(LINUX_TARGET) $(LINUX_LIBRARY)

avr: bin $(AVR_TARGET) $(AVR_LIBRARY)

bin:
	@mkdir $(BIN)

2cstr:
	$(LINUX_CC) -std=c11 $(SRC)/2cstr.c -o 2cstr

clean: clean_linux clean_avr
	rm -f 2cstr


###################
#      LINUX      #
###################

CRITERION_BUILD=$(TEMP_LINUX)/Criterion
CRITERION_LIB=$(CRITERION_BUILD)/libcriterion.so

$(LINUX_LIBRARY): $(LINUX_FILES) $(LINUX_GENERATED)
	rm -f "$@"
	$(LINUX_AR) scr $@ $^

$(LINUX_TARGET): $(TEMP_LINUX)/$(MAIN_FILE) | $(LINUX_LIBRARY)
	# First pass, won't remove unused symbols because it will supress undefined reference errors
	# in unused symbols and they are used for dependendy checking
	$(LINUX_CC) $(LINUX_OPTIONS) $^ -Wl,--whole-archive -l$(LINUX_NAME) -Wl,--no-whole-archive $(LINUX_LIBS) -o $(LINUX_TARGET)_tmp
	rm $(LINUX_TARGET)_tmp
	# Second pass, there isn't any undefined reference, remove all unused symbols
	$(LINUX_CC) $(LINUX_OPTIONS) -Wl,-gc-sections $^ -Wl,--whole-archive -l$(LINUX_NAME) -Wl,--no-whole-archive $(LINUX_LIBS) -o $@

$(TEMP_LINUX)/%.o: $(SRC)/%.c $(HEADERS)
	@mkdir -p "$(shell dirname "$@")"
	$(LINUX_CC) $(LINUX_OPTIONS) -c $< -o $@

$(TEMP_LINUX)/%.g1.o: $(TEMP_LINUX)/%.g1.c $(TEMP_LINUX)/%.g1.h
	$(LINUX_CC) $(LINUX_OPTIONS) -Wno-overlength-strings -c $< -o $@

$(TEMP_LINUX)/%.g1.c $(TEMP_LINUX)/%.g1.h: 2cstr
	./genCode.sh "$(shell basename $@ | head -c -6)" "$(shell basename $@ | head -c -6)" "$(URL_FILE_BASE)" "$(TEMP_LINUX)" "$(GEN_DEST)" "$(shell basename $@ | head -c -6)"

clean_linux:
	rm -rf "$(TEMP_LINUX)" "$(LINUX_TARGET)" "$(LINUX_LIBRARY)"

$(CRITERION_LIB):
	git submodule update "$(CRITERION_SRC)"
	@mkdir -p "$(CRITERION_BUILD)"
	cd "$(CRITERION_BUILD)" && cmake "$(PROJECT_DIR)/$(CRITERION_SRC)"
	make -C "$(CRITERION_BUILD)"

$(TEMP_LINUX)/test/%.o: $(SRC)/test/%.c
	@mkdir -p "$(shell dirname "$@")"
	$(LINUX_CC) $(LINUX_OPTIONS) -I"$(LIB)/Criterion/include/" -c $^ -o $@

$(TEMP_LINUX)/test_objs/%.o: $(SRC)/%.c $(HEADERS)
	@mkdir -p "$(shell dirname "$@")"
	$(LINUX_CC) $(TEST_OPTIONS) -c $< -o $@

.SECONDEXPANSION:
$(TEMP_LINUX)/test/%: $(TEMP_LINUX)/test/%.o $$(addprefix $(TEMP_LINUX)/test_objs/,$$(addsuffix .o,$$(shell grep 'DEPENDS\:' "$(SRC)/test/$$*.c" | sed 's|^.*DEPENDS\:\s*||'))) | $(CRITERION_LIB)
	$(LINUX_CC) $(TEST_OPTIONS) -L"$(CRITERION_BUILD)" $^ $(LINUX_LIBS) -lcriterion -o $@

test-%: $(TEMP_LINUX)/test/%
	@echo "Test '$*':"
	LD_LIBRARY_PATH="$(CRITERION_BUILD)" $^ --no-early-exit
	gcov -rn $(addprefix $(TEMP_LINUX)/,$(addsuffix .gcda,$(shell grep 'TEST FOR:' "$(SRC)/test/$*.c" | sed 's|^.*TEST FOR:\s*||')))

test:
	i=0; \
	for test in $(TESTS); \
	do if ! make "$$test"; \
          then i=$$(expr $$i + 1); \
	fi; done; \
	exit $$i

###################
#       AVR       #
###################

$(AVR_LIBRARY): $(AVR_FILES) $(AVR_GENERATED)
	rm -f "$@"
	$(AVR_AR) scr $@ $^

$(AVR_TARGET): $(TEMP_AVR)/$(MAIN_FILE) | $(AVR_LIBRARY)
	$(AVR_CC) $(AVR_OPTIONS) $^ -Wl,--whole-archive -l$(AVR_NAME) -Wl,--no-whole-archive -o $@
	avr-size -C --mcu=atmega1284 $@

$(TEMP_AVR)/%.o: $(SRC)/%.c $(HEADERS)
	@mkdir -p "$(shell dirname "$@")"
	$(AVR_CC) $(AVR_OPTIONS) -c $< -o $@

$(TEMP_AVR)/%.g1.o: $(TEMP_AVR)/%.g1.c $(TEMP_AVR)/%.g1.h
	$(AVR_CC) $(AVR_OPTIONS) -Wno-overlength-strings -c $< -o $@

$(TEMP_AVR)/%.g1.c $(TEMP_AVR)/%.g1.h: 2cstr
	./genCode.sh "$(shell basename $@ | head -c -6)" "$(shell basename $@ | head -c -6)" "$(URL_FILE_BASE)" "$(TEMP_AVR)" "$(GEN_DEST)" "$(shell basename $@ | head -c -6)"

clean_avr:
	rm -rf "$(TEMP_AVR)" "$(AVR_TARGET)"


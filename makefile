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
LINUX_AR=ar
AVR_CC=avr-gcc
AVR_AR=avr-ar
AVR_MCU=atmega16
AVR_F_CPU = 3686400UL

OPTIONS        += -std=c11 #--short-enums
OPTIONS        += -I$(SRC)/headers/ -L$(BIN)
OPTIONS        += -Wall -Wextra -pedantic -Werror

LINUX_LIBS=

NO_TCP = 1

test-% test: OPTIONS += -fprofile-arcs -ftest-coverage -fsanitize=undefined
test-% test: LINUX_LIBS += -lgcov

OPTIONS        += $(O)

ifdef SANITIZE
OPTIONS        += -fsanitize=undefined
endif

ifndef_any_of = $(filter undefined,$(foreach v,$(1),$(origin $(v))))
ifdef_any_of = $(filter-out undefined,$(foreach v,$(1),$(origin $(v))))

ifneq ($(call ifdef_any_of,DEBUG SANITIZE),)
OPTIONS        += -Og -g -DDEBUG
else
OPTIONS        += -Os #-flto
OPTIONS        += -ffast-math
OPTIONS        += -s -ffunction-sections -fdata-sections
endif

PROJECT_DIR=$(PWD)

GEN_DEST = FilesAsCArrays
URL_FILE_BASE   = 

OPTIONAL_FILES  += server/protocol/icmp.o
OPTIONAL_FILES  += server/adelay.o
OPTIONAL_FILES  += server/ressource.o

ifndef NO_TCP
ifndef NO_HTTP
OPTIONAL_FILES  += server/services/http.o
endif
OPTIONAL_FILES  += server/protocol/tcp.o
OPTIONAL_FILES  += server/protocol/tcp_stack.o
OPTIONAL_FILES  += server/protocol/tcp_retransmission_cache.o
OPTIONS += -DUSE_TCP
endif

ifndef NO_FILE_RESSOURCES
OPTIONAL_FILES  += server/fileRessource.o
RESSOURCE_GETTER += X(FileRessource)
endif

ifdef RESSOURCE_GETTER
OPTIONS += -DRESSOURCE_GETTER="$(RESSOURCE_GETTER)"
endif

OPTIONAL_FILES += server/protocol/IPv4.o

MAIN_FILE = main.o

FILES += server/server.o
FILES += utils/utils.o
FILES += utils/ringbuffer.o
FILES += utils/stream.o
FILES += utils/mempool.o
FILES += server/checksum.o
FILES += server/protocol/arp.o
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
LINUX_NAME	= dpaucs-linux
LINUX_TARGET	= $(BIN)/$(LINUX_NAME)
LINUX_LIBRARY   = $(BIN)/lib$(LINUX_NAME).a
LINUX_GENERATED = $(shell find ${TEMP_LINUX}/${GEN_DEST} -iname "*.o")

LINUX_OPTIONS  += -I${TEMP_LINUX}

LINUX_FILES_TMP  = $(FILES)
LINUX_FILES_TMP += server/drivers/eth/linux.o
LINUX_FILES_TMP += server/drivers/adelay_clock.o
LINUX_FILES_TMP += utils/logger.o

LINUX_FILES = $(shell \
  for file in ${LINUX_FILES_TMP}; \
  do \
    echo "${TEMP_LINUX}/$$file"; \
  done; \
)

CRITERION_SRC=$(LIB)/Criterion
TESTS=$(shell \
  for file in "$(SRC)"/test/*.c; do \
    file="$(TEMP_LINUX)/test/$$(basename "$$file" .c).o"; \
    echo "$$file"; \
  done \
)

###################
#       AVR       #
###################

AVR_OPTIONS	= $(OPTIONS) -DF_CPU=$(AVR_F_CPU) -mmcu=$(AVR_MCU)
TEMP_AVR	= tmp/avr_$(AVR_MCU)
AVR_NAME	= dpaucs-$(AVR_MCU)
AVR_TARGET	= $(BIN)/$(AVR_NAME)
AVR_LIBRARY	= $(BIN)/lib$(AVR_NAME).a
AVR_GENERATED   = $(shell find ${TEMP_LINUX}/${GEN_DEST} -iname "*.o")

AVR_OPTIONS  += -I${TEMP_AVR}

AVR_FILES_TMP  = $(FILES)
AVR_FILES_TMP += server/drivers/eth/dummy.o

ifndef NO_ADELAY_DRIVER
AVR_FILES_TMP   += server/drivers/adelay_timer.o
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
	$(LINUX_CC) $(LINUX_OPTIONS) $^  -Wl,--whole-archive -l$(LINUX_NAME) -Wl,--no-whole-archive $(LINUX_LIBS) -o $(LINUX_TARGET)_tmp
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

$(TEMP_LINUX)/test/%: $(TEMP_LINUX)/test/%.o $(LINUX_LIBRARY) | $(CRITERION_LIB)
	$(LINUX_CC) $(LINUX_OPTIONS) -L"$(CRITERION_BUILD)" $^ $(LINUX_LIBS) -lcriterion -o $@

test-%: $(TEMP_LINUX)/test/%
	LD_LIBRARY_PATH="$(CRITERION_BUILD)" $^ --no-early-exit
	gcov -rn $(addprefix $(TEMP_LINUX)/,$(addsuffix .gcda,$(shell grep '^//\s*TEST FOR:' "$(SRC)/test/$*.c" | sed 's|^//\s*TEST FOR:\s*||')))

$(TEMP_LINUX)/test/test-all: $(TESTS) $(LINUX_LIBRARY) | $(CRITERION_LIB)
	$(LINUX_CC) $(LINUX_OPTIONS) -L"$(CRITERION_BUILD)" $^ $(LINUX_LIBS) -lcriterion -o $@

test: $(TEMP_LINUX)/test/test-all
	LD_LIBRARY_PATH="$(CRITERION_BUILD)" $^ --no-early-exit
	gcov -rn $(addprefix $(TEMP_LINUX)/,$(addsuffix .gcda,$(shell grep -hr '^//\s*TEST FOR:' "$(SRC)/test/" | sed 's|^//\s*TEST FOR:\s*||')))

###################
#       AVR       #
###################

$(AVR_LIBRARY): $(AVR_FILES) $(AVR_GENERATED)
	rm -f "$@"
	$(AVR_AR) scr $@ $^

$(AVR_TARGET): $(TEMP_AVR)/$(MAIN_FILE) | $(AVR_LIBRARY)
	$(AVR_CC) $(AVR_OPTIONS) $^ -Wl,--whole-archive -l$(AVR_NAME) -Wl,--no-whole-archive -o $@

$(TEMP_AVR)/%.o: $(SRC)/%.c $(HEADERS)
	@mkdir -p "$(shell dirname "$@")"
	$(AVR_CC) $(AVR_OPTIONS) -c $< -o $@

$(TEMP_AVR)/%.g1.o: $(TEMP_AVR)/%.g1.c $(TEMP_AVR)/%.g1.h
	$(AVR_CC) $(AVR_OPTIONS) -Wno-overlength-strings -c $< -o $@

$(TEMP_AVR)/%.g1.c $(TEMP_AVR)/%.g1.h: 2cstr
	./genCode.sh "$(shell basename $@ | head -c -6)" "$(shell basename $@ | head -c -6)" "$(URL_FILE_BASE)" "$(TEMP_AVR)" "$(GEN_DEST)" "$(shell basename $@ | head -c -6)"

clean_avr:
	rm -rf "$(TEMP_AVR)" "$(AVR_TARGET)"


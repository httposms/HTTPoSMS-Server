include config.mk

SRC_DIR = src
OBJ_DIR = obj
SRC = $(wildcard ${SRC_DIR}/*.c)
OBJ = $(patsubst ${SRC_DIR}/%.c,${OBJ_DIR}/%.o,${SRC})

all: options ${EXEC}

options:
	@echo Test build options
	@echo "CFLAGS     = ${CFLAGS}"
	@echo "LDFLAGS    = ${LDFLAGS}"
	@echo "CC         = ${CC}"
	@echo "SRC        = ${SRC}"
	@echo "OBJ        = ${OBJ}"
	@echo "[TEST] OBJ = ${A_OBJ}"

${EXEC}: ${A_OBJ} ${OBJ}	
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS}

${OBJ}: | ${OBJ_DIR}

${OBJ_DIR}:
	mkdir $@

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c
	${CC} -c ${CFLAGS} $< -o $@ ${LDFLAGS}

.PHONY: clean
clean:
	rm -rf ${EXEC} ${OBJ_DIR}

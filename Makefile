include config.mk

.PHONY: options
MAIN = ${OBJ_DIR}/driver.o
SRC = $(wildcard ${SRC_DIR}/*.c)
OBJ = $(patsubst ${SRC_DIR}/%.c,${OBJ_DIR}/%.o,${SRC})

all: options ${NAME}

debug: CFLAGS += -g
debug: all

options:
	@echo ""
	@echo "=====Build options====="
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@echo "SRC      = ${SRC}"
	@echo "OBJ      = ${OBJ}"
	@echo ""

${NAME}: ${OBJ}
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS}

# The pipe is pretty cool
${OBJ}: | ${OBJ_DIR}

${OBJ_DIR}:
	mkdir $@

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c
	${CC} -c ${CFLAGS} $< -o $@ 
 
test: ${OBJ}
	$(MAKE) -C test/ A_INCLUDE=${INCLUDE} A_OBJ="$(patsubst %,../%,$(filter-out ${MAIN},${OBJ}))" A_LDFLAGS="${LDFLAGS}"
A_LDFLAGS=${LDFLAGS}

sdist:
	mkdir -p ${NAME}-${VERSION}
	cp -R LICENSE Makefile config.mk ${SRC_DIR} ${NAME}-${VERSION}
	tar -cvzf ${NAME}-${VERSION}.tar.gz ${NAME}-${VERSION}

clean:
	$(MAKE) -C test/ clean
	rm -rf ${OBJ_DIR} ${NAME}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${NAME} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${NAME}
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < docs/${NAME}.1 > ${DESTDIR}${MANPREFIX}/man1/${NAME}.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/${NAME}.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${NAME}\
		${DESTDIR}${MANPREFIX}/man1/${NAME}.1

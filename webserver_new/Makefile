# ========= DIRECTORIES =============
SRC_DIR = src/
OBJ_DIR = obj/
HEADERS_DIR = include/

# ========== HEADER FILES ============

HEADER_FILE =	${addprefix ${HEADERS_DIR}, ${MAIN_HEADER_FILE}} \
				${addprefix ${HEADERS_DIR}${CLASS_HEADER_DIR}, ${CLASS_HEADER_FILE}}

CLASS_HEADER_DIR = classes/
CLASS_HEADER_FILE =	ConfigData.hpp \
					FileDescriptor.hpp \
					Http.hpp \
					HttpCgiData.hpp \
					HttpRequest.hpp \
					Logger.hpp \
					ServerConfig.hpp \
					Socket.hpp \
					WebServ.hpp \
					WebservException.hpp \
					CharTable.hpp

MAIN_HEADER_FILE =	defined_value.hpp \
					utility_function.hpp

# ========== SOURCE  FILES ===========

SRC_MAIN =	${addprefix ${SRC_DIR}, ${SOURCE_MAIN_FILE}} \
			${addprefix ${SRC_DIR}${SRC_CLASS_DIR}, ${SRC_CLASSES_FILE}}

SRC_TEST =	${addprefix ${SRC_DIR}, ${SOURCE_TEST_FILE}} \
			${addprefix ${SRC_DIR}${SRC_CLASS_DIR}, ${SRC_CLASSES_FILE}}

SRC_CLASS_DIR = classes_definition/
SRC_CLASSES_FILE =	ConfigData.cpp \
					FileDescriptor.cpp \
					Http.cpp \
					HttpCgiData.cpp \
					HttpRequest.cpp \
					Logger.cpp \
					ServerConfig.cpp \
					Socket.cpp \
					WebServ.cpp \
					WebservException.cpp \
					CharTable.cpp

SOURCE_MAIN_FILE = main.cpp

SOURCE_TEST_FILE = unittest.cpp

# =========== OBJECT FILES ==========

OBJ_MAIN = ${patsubst ${SRC_DIR}%.cpp, ${OBJ_DIR}%.o, ${SRC_MAIN}}

OBJ_TEST = ${patsubst ${SRC_DIR}%.cpp, ${OBJ_DIR}%.o, ${SRC_TEST}}


# ========== COMPILE DIRECTIVES ======
CC = c++

CFLAGS = -std=c++98 -pedantic-errors -I./${HEADERS_DIR} #-Wall -Wextra -Werror

NAME = Webserv
TEST_NAME = ${NAME}_unittest

all: ${NAME}

${NAME}: ${OBJ_MAIN}
	${CC} ${CFLAGS} ${OBJ_MAIN} -o $@

${OBJ_DIR}:
	@mkdir -p $@

${OBJ_DIR}%.o: ${SRC_DIR}%.cpp ${HEADER_FILE} Makefile | ${OBJ_DIR}
	@mkdir -p ${dir $@}
	${CC} ${CFLAGS} -c $< -o $@

${TEST_NAME}: ${OBJ_TEST}
	${CC} ${CFLAGS} ${OBJ_TEST} -o $@

clean:
	rm -rf ${OBJ_DIR}

fclean: clean
	rm -rf ${NAME} ${TEST_NAME}

re: fclean all

test: ${TEST_NAME}
	valgrind --tool=memcheck \
			--leak-check=full \
			--show-leak-kinds=all \
			--errors-for-leak-kinds=all \
			--track-origins=yes \
			--track-fds=yes \
			--trace-children=yes \
			--error-exitcode=1 \
			./${TEST_NAME}

.PHONY: all clean fclean re test
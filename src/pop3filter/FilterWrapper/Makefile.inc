
CC       = clang
# Compiling Flags:
CFLAGS   = -c -g --std=c99 -pedantic -pedantic-errors -Wall -Wextra -Werror -Wno-unused-parameter -Wno-implicit-fallthrough -D_POSIX_C_SOURCE=200809L -I./include -I./../Parsers/include -I./../include -I./../../Utils/include

LINKER 	 = clang
# Linking Flags:
LFLAGS 	 = -g --std=c99 -pedantic -pedantic-errors -Wall -Wextra -Werror -Wno-unused-parameter -Wno-implicit-fallthrough -D_POSIX_C_SOURCE=200809L  -lpthread -pthread -lsctp



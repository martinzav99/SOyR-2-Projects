.PHONY: all clean

SRC = read_boot.c read_mbr.c read_root.c recover_file.c
BIN = $(SRC:.c=)

all: $(BIN)

%: %.c
	gcc $< -o $@

clean:
	rm -f $(BIN)

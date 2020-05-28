# Makefile for texture mangler
# 05-28-2020 E. Brombaugh

CC = gcc

# output binary name
BINARY = mk_textures

# output file
OUTPUT = textures.h

# Targets
all: $(OUTPUT)

$(BINARY): mk_textures.c
	$(CC) -o $@ $<

$(OUTPUT): $(BINARY)
	./$(BINARY)

clean:
	rm -f $(BINARY) $(OUTPUT)
	

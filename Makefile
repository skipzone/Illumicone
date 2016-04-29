
all: bins

bins:
	$(MAKE) -C src/

clean:
	$(MAKE) -C src/ clean

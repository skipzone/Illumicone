
all: bins

bins:
	$(MAKE) -C src/

install:
	$(MAKE) -C src/ install

clean:
	$(MAKE) -C src/ clean

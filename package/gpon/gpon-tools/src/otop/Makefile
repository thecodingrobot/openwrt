# otop - optic debug TUI (GPL, from gpon_optic_drv 4.5.0 tools/otop)
otop: optic_top.o dump.o tui.o
	$(CC) $(BASE_CFLAGS) $(OPTIC_INC) -o $@ $^
%.o: %.c
	$(CC) $(BASE_CFLAGS) $(OPTIC_INC) -DPACKAGE_VERSION='"4.5.0"' -c -o $@ $<
clean:
	rm -f otop *.o

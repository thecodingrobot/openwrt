# gtop - ONU debug TUI (GPL, from gpon_onu_drv 4.5.0 tools/gtop)
OBJS = gtop.o common.o dump.o tui.o gtc_counter.o gpe_counter.o sce_counter.o
gtop: $(OBJS) onu_control_rpc.o
	$(CC) $(BASE_CFLAGS) $(ONU_INC) -o $@ $^
%.o: %.c
	$(CC) $(BASE_CFLAGS) $(ONU_INC) -c -o $@ $<
onu_control_rpc.o: ../onu/onu_control_rpc.c
	$(CC) $(BASE_CFLAGS) -I../onu $(ONU_INC) -c -o $@ $<
clean:
	rm -f gtop *.o

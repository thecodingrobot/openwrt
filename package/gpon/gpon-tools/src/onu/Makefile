# onu - ONU driver control CLI (GPL, from gpon_onu_drv 4.5.0 tools/onu)
OBJS = onu_control.o onu_control_cli.o onu_control_rpc.o onu_sce_wrappers_common.o onu_sce_wrappers_misc.o onu_sce_json_wrappers.o onu_sce_xml_wrappers.o
onu: $(OBJS)
	$(CC) $(BASE_CFLAGS) $(ONU_INC) -o $@ $(OBJS)
%.o: %.c
	$(CC) $(BASE_CFLAGS) $(ONU_INC) -c -o $@ $<
clean:
	rm -f onu *.o

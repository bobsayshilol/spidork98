# Compilation rules
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.cc $(DEPS)
	$(CC) $(CFLAGS) -x c++ -c -o $@ $<

# Make an EXE
$(TARGET): $(OBJS) $(DEPS)
	$(LD) -o $@ $(OBJS) $(LIBPC98)
	$(STRIP) $@

# Cleanup
.PHONY: clean
clean:
	del *.o
	del $(TARGET)

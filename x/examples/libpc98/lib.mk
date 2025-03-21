# Compilation rules
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.cc $(DEPS)
	$(CC) $(CFLAGS) -x c++ -c -o $@ $<

# Make a LIB
$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

# Cleanup
.PHONY: clean
clean:
	del *.o
	del $(TARGET)

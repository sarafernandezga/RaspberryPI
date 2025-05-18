LIBS= -lpthread -lm #Libraries used if needed
SRCS= SensorColorMPU2.c
BIN=app
CFLAGS+= -g -O0
OBJS=$(subst .cpp,.o,$(SRCS))
all : $(BIN)
$(BIN): $(OBJS)
	@echo [link] $@
	$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
%.o: %.cpp
	@echo [Compile] $<
	$(CXX) -c $(CFLAGS) $< -o $@
clean:
	@rm -f $(BIN)

CXX = g++
CXXFLAGS =-Wall -std=c++11 -Iinc -g `pkg-config --cflags protobuf grpc`
LDFLAGS = -L/usr/local/lib -lm -ldl -lpthread `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed

PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = protos
vpath %.proto $(PROTOS_PATH)

# All the cpp file in the directory should be compiled
src = $(wildcard *.cpp)
obj = $(patsubst %.cpp, obj/%.o, $(src)) # Each cpp file will trun into an object file in the obj folder

obj2 = $(filter-out obj/userprogram.o,  $(obj)) # Object files required for the server application. It contains all the object files except the object file of userprogram which should be used for client
obj1 = $(filter-out obj/server_driver.o, $(obj)) # Object files for userprogram application. Please note that Client application created by this Makefile will be the one that runs the main function in the userprogram object file

.PHONY: all
all: system-check obj Client Server libProject1.a

client : obj Client
Client: key_store_services.pb.o key_store_services.grpc.pb.o utils.o client_lib.o userprogram_cm_test1.o
	$(CXX) -g -o $@ $^ $(LDFLAGS) # Link object files and create the client application

server: obj Server
Server: key_store_services.pb.o key_store_services.grpc.pb.o cm_services.pb.o cm_services.grpc.pb.o utils.o cm_client.o cm_server.o server.o
	$(CXX) -g -o $@ $^ $(LDFLAGS) # Link object files and create the server application

libProject1.a: key_store_services.pb.o key_store_services.grpc.pb.o utils.o client_lib.o
	ar rcs libProject1.a client_lib.o

obj:
	mkdir obj # Create a folder for the object files

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

# Compile the code and create object files
$(obj): obj/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf obj Client Server *.o

PROTOC_CMD = which $(PROTOC)
PROTOC_CHECK_CMD = $(PROTOC) --version | grep -q libprotoc.3
PLUGIN_CHECK_CMD = which $(GRPC_CPP_PLUGIN)
HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)
ifeq ($(HAS_PROTOC),true)
HAS_VALID_PROTOC = $(shell $(PROTOC_CHECK_CMD) 2> /dev/null && echo true || echo false)
endif
HAS_PLUGIN = $(shell $(PLUGIN_CHECK_CMD) > /dev/null && echo true || echo false)

SYSTEM_OK = false
ifeq ($(HAS_VALID_PROTOC),true)
ifeq ($(HAS_PLUGIN),true)
SYSTEM_OK = true
endif
endif


system-check:
ifneq ($(HAS_VALID_PROTOC),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have protoc 3.0.0 installed in your path."
	@echo "Please install Google protocol buffers 3.0.0 and its compiler."
	@echo "You can find it here:"
	@echo
	@echo "   https://github.com/google/protobuf/releases/tag/v3.0.0"
	@echo
	@echo "Here is what I get when trying to evaluate your version of protoc:"
	@echo
	-$(PROTOC) --version
	@echo
	@echo
endif
ifneq ($(HAS_PLUGIN),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have the grpc c++ protobuf plugin installed in your path."
	@echo "Please install grpc. You can find it here:"
	@echo
	@echo "   https://github.com/grpc/grpc"
	@echo
	@echo "Here is what I get when trying to detect if you have the plugin:"
	@echo
	-which $(GRPC_CPP_PLUGIN)
	@echo
	@echo
endif
ifneq ($(SYSTEM_OK),true)
	@false
endif

#installing GRPC
#export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
# sudo apt install -y cmake build-essential autoconf libtool pkg-config clang libssl-dev git
# git clone --recurse-submodules -b v1.33.2 https://github.com/grpc/grpc
# execute full https://github.com/grpc/grpc/blob/master/test/distrib/cpp/run_distrib_test_cmake.sh
# 

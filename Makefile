CC = gcc
LINK = $(CC)
AR = ar

SRC = ./src
BUILD = ./build
TEST = ./tests

CFLAGS = -std=gnu99 -g -Wall
#CFLAGS = -std=gnu99 -Wall -O3 -fno-strict-aliasing

INCS = -I$(SRC) \
	-I/usr/include/mysql

LIBS = -L/usr/lib/mysql

LDLIBS =  -lev -lcurl -lJudy -lmysqlclient -pthread

DEPS = $(SRC)/cor_array.h \
	$(SRC)/cor_core.h \
	$(SRC)/cor_config.h \
	$(SRC)/cor_buf.h \
	$(SRC)/cor_dict.h \
	$(SRC)/cor_html.h \
	$(SRC)/cor_http.h \
	$(SRC)/cor_kuku.h \
	$(SRC)/cor_list.h \
	$(SRC)/cor_log.h \
	$(SRC)/cor_mmap.h \
	$(SRC)/cor_morph.h \
	$(SRC)/cor_mysql.h \
	$(SRC)/cor_pool.h \
	$(SRC)/cor_smap.h \
	$(SRC)/cor_spider.h \
	$(SRC)/cor_str.h \
	$(SRC)/cor_thread.h \
	$(SRC)/cor_time.h \
	$(SRC)/cor_trace.h \
	$(SRC)/cor_uuid.h \
	$(SRC)/xxhash.h \
	$(TEST)/cor_test.h

OBJS = $(BUILD)/cor_array.o \
	$(BUILD)/cor_buf.o \
	$(BUILD)/cor_config.o \
	$(BUILD)/cor_dict.o \
	$(BUILD)/cor_html.o \
	$(BUILD)/cor_http.o \
	$(BUILD)/cor_kuku.o \
	$(BUILD)/cor_list.o \
	$(BUILD)/cor_log.o \
	$(BUILD)/cor_mmap.o \
	$(BUILD)/cor_morph.o \
	$(BUILD)/cor_mysql.o \
	$(BUILD)/cor_pool.o \
	$(BUILD)/cor_smap.o \
	$(BUILD)/cor_spider.o \
	$(BUILD)/cor_str.o \
	$(BUILD)/cor_thread.o \
	$(BUILD)/cor_time.o \
	$(BUILD)/cor_trace.o \
	$(BUILD)/cor_uuid.o \
	$(BUILD)/xxhash.o

LIBS = $(BUILD)/libcore.a

TESTS = $(BUILD)/test_cor_buf \
	$(BUILD)/test_cor_html \
	$(BUILD)/test_cor_http \
	$(BUILD)/test_cor_list \
	$(BUILD)/test_cor_log \
	$(BUILD)/test_cor_morph \
	$(BUILD)/test_cor_mysql \
	$(BUILD)/test_cor_smap \
	$(BUILD)/test_cor_spider \
	$(BUILD)/test_cor_thread \
	$(BUILD)/test_cor_trace \
	$(BUILD)/test_cor_uuid

all: prebuild $(OBJS) $(LIBS) $(TESTS)

$(BUILD)/libcore.a: $(OBJS)
	$(AR) rcs $@ $^

$(BUILD)/cor_array.o: $(DEPS) \
	$(SRC)/cor_array.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_array.o $(SRC)/cor_array.c

$(BUILD)/cor_buf.o: $(DEPS) \
	$(SRC)/cor_buf.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_buf.o $(SRC)/cor_buf.c

$(BUILD)/cor_config.o: $(DEPS) \
	$(SRC)/cor_config.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_config.o $(SRC)/cor_config.c

$(BUILD)/cor_dict.o: $(DEPS) \
	$(SRC)/cor_dict.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_dict.o $(SRC)/cor_dict.c

$(BUILD)/cor_html.o: $(DEPS) \
	$(SRC)/cor_html.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_html.o $(SRC)/cor_html.c

$(BUILD)/cor_http.o: $(DEPS) \
	$(SRC)/cor_http.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_http.o $(SRC)/cor_http.c

$(BUILD)/cor_kuku.o: $(DEPS) \
	$(SRC)/cor_kuku.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_kuku.o $(SRC)/cor_kuku.c

$(BUILD)/cor_list.o: $(DEPS) \
	$(SRC)/cor_list.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_list.o $(SRC)/cor_list.c

$(BUILD)/cor_log.o: $(DEPS) \
	$(SRC)/cor_log.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_log.o $(SRC)/cor_log.c

$(BUILD)/cor_mmap.o: $(DEPS) \
	$(SRC)/cor_mmap.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_mmap.o $(SRC)/cor_mmap.c

$(BUILD)/cor_morph.o: $(DEPS) \
	$(SRC)/cor_morph.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_morph.o $(SRC)/cor_morph.c

$(BUILD)/cor_mysql.o: $(DEPS) \
	$(SRC)/cor_mysql.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_mysql.o $(SRC)/cor_mysql.c

$(BUILD)/cor_pool.o: $(DEPS) \
	$(SRC)/cor_pool.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_pool.o $(SRC)/cor_pool.c

$(BUILD)/cor_smap.o: $(DEPS) \
	$(SRC)/cor_smap.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_smap.o $(SRC)/cor_smap.c

$(BUILD)/cor_spider.o: $(DEPS) \
	$(SRC)/cor_spider.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_spider.o $(SRC)/cor_spider.c

$(BUILD)/cor_str.o: $(DEPS) \
	$(SRC)/cor_str.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_str.o $(SRC)/cor_str.c

$(BUILD)/cor_thread.o: $(DEPS) \
	$(SRC)/cor_thread.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_thread.o $(SRC)/cor_thread.c

$(BUILD)/cor_time.o: $(DEPS) \
	$(SRC)/cor_time.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_time.o $(SRC)/cor_time.c

$(BUILD)/cor_trace.o: $(DEPS) \
	$(SRC)/cor_trace.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_trace.o $(SRC)/cor_trace.c

$(BUILD)/cor_uuid.o: $(DEPS) \
	$(SRC)/cor_uuid.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/cor_uuid.o $(SRC)/cor_uuid.c

$(BUILD)/xxhash.o: $(DEPS) \
	$(SRC)/xxhash.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/xxhash.o $(SRC)/xxhash.c

$(BUILD)/test_cor_buf: \
	$(BUILD)/test_cor_buf.o $(BUILD)/cor_buf.o $(BUILD)/cor_list.o
	$(LINK) -o $(BUILD)/test_cor_buf $(LIBS) $(BUILD)/test_cor_buf.o $(BUILD)/cor_buf.o $(BUILD)/cor_list.o $(LDLIBS)

$(BUILD)/test_cor_html: \
	$(BUILD)/test_cor_html.o $(BUILD)/cor_html.o $(BUILD)/cor_log.o $(BUILD)/cor_pool.o $(BUILD)/cor_str.o
	$(LINK) -o $(BUILD)/test_cor_html $(LIBS) $(BUILD)/test_cor_html.o $(BUILD)/cor_log.o $(BUILD)/cor_pool.o $(BUILD)/cor_str.o $(LDLIBS)

$(BUILD)/test_cor_http: \
	$(BUILD)/test_cor_http.o $(BUILD)/cor_array.o $(BUILD)/cor_buf.o $(BUILD)/cor_list.o $(BUILD)/cor_log.o $(BUILD)/cor_pool.o $(BUILD)/cor_spider.o $(BUILD)/cor_str.o
	$(LINK) -o $(BUILD)/test_cor_http $(LIBS) $(BUILD)/test_cor_http.o $(BUILD)/cor_array.o $(BUILD)/cor_buf.o $(BUILD)/cor_list.o $(BUILD)/cor_log.o $(BUILD)/cor_str.o $(BUILD)/cor_pool.o $(BUILD)/cor_spider.o $(LDLIBS)

$(BUILD)/test_cor_list: \
	$(BUILD)/test_cor_list.o $(BUILD)/cor_list.o 
	$(LINK) -o $(BUILD)/test_cor_list $(BUILD)/test_cor_list.o $(BUILD)/cor_list.o $(LDLIBS)

$(BUILD)/test_cor_log: \
	$(BUILD)/test_cor_log.o $(BUILD)/cor_log.o 
	$(LINK) -o $(BUILD)/test_cor_log $(BUILD)/test_cor_log.o $(BUILD)/cor_log.o $(LDLIBS)

$(BUILD)/test_cor_morph: \
	$(BUILD)/test_cor_morph.o $(BUILD)/cor_dict.o $(BUILD)/cor_log.o $(BUILD)/cor_mmap.o $(BUILD)/cor_morph.o $(BUILD)/cor_pool.o $(BUILD)/cor_str.o $(BUILD)/cor_time.o
	$(LINK) -o $(BUILD)/test_cor_morph $(BUILD)/test_cor_morph.o $(BUILD)/cor_dict.o $(BUILD)/cor_log.o $(BUILD)/cor_mmap.o $(BUILD)/cor_morph.o $(BUILD)/cor_pool.o $(BUILD)/cor_str.o $(BUILD)/cor_time.o $(LDLIBS)

$(BUILD)/test_cor_mysql: \
	$(BUILD)/test_cor_mysql.o
	$(LINK) -o $(BUILD)/test_cor_mysql $(BUILD)/test_cor_mysql.o $(LDLIBS)

$(BUILD)/test_cor_smap: \
	$(BUILD)/test_cor_smap.o $(BUILD)/xxhash.o $(BUILD)/cor_time.o
	$(LINK) -o $(BUILD)/test_cor_smap $(LIBS) $(BUILD)/test_cor_smap.o $(BUILD)/xxhash.o $(BUILD)/cor_time.o

$(BUILD)/test_cor_spider: \
	$(BUILD)/test_cor_spider.o $(BUILD)/cor_array.o $(BUILD)/cor_buf.o $(BUILD)/cor_list.o $(BUILD)/cor_log.o $(BUILD)/cor_http.o $(BUILD)/cor_pool.o $(BUILD)/cor_spider.o $(BUILD)/cor_str.o
	$(LINK) -o $(BUILD)/test_cor_spider $(LIBS) $(BUILD)/test_cor_spider.o $(BUILD)/cor_array.o $(BUILD)/cor_buf.o $(BUILD)/cor_list.o $(BUILD)/cor_log.o $(BUILD)/cor_http.o $(BUILD)/cor_pool.o $(BUILD)/cor_spider.o $(BUILD)/cor_str.o  $(LDLIBS)

$(BUILD)/test_cor_thread: \
	$(BUILD)/test_cor_thread.o $(BUILD)/cor_thread.o
	$(LINK) -o $(BUILD)/test_cor_thread $(LIBS) $(BUILD)/test_cor_thread.o $(BUILD)/cor_thread.o $(LDLIBS)

$(BUILD)/test_cor_trace: \
	$(BUILD)/test_cor_trace.o $(BUILD)/cor_trace.o
	$(LINK) -o $(BUILD)/test_cor_trace $(LIBS) $(BUILD)/test_cor_trace.o $(BUILD)/cor_trace.o $(LDLIBS)

$(BUILD)/test_cor_uuid: \
	$(BUILD)/test_cor_uuid.o $(BUILD)/cor_uuid.o
	$(LINK) -o $(BUILD)/test_cor_uuid $(LIBS) $(BUILD)/test_cor_uuid.o $(BUILD)/cor_uuid.o $(LDLIBS)

$(BUILD)/test_cor_buf.o: $(DEPS) \
	$(TEST)/test_cor_buf.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_buf.o $(TEST)/test_cor_buf.c

$(BUILD)/test_cor_html.o: $(DEPS) \
	$(TEST)/test_cor_html.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_html.o $(TEST)/test_cor_html.c

$(BUILD)/test_cor_http.o: $(DEPS) \
	$(TEST)/test_cor_http.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_http.o $(TEST)/test_cor_http.c

$(BUILD)/test_cor_list.o: $(DEPS) \
	$(TEST)/test_cor_list.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_list.o $(TEST)/test_cor_list.c

$(BUILD)/test_cor_log.o: $(DEPS) \
	$(TEST)/test_cor_log.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_log.o $(TEST)/test_cor_log.c

$(BUILD)/test_cor_morph.o: $(DEPS) \
	$(TEST)/test_cor_morph.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_morph.o $(TEST)/test_cor_morph.c

$(BUILD)/test_cor_mysql.o: $(DEPS) \
	$(TEST)/test_cor_mysql.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_mysql.o $(TEST)/test_cor_mysql.c

$(BUILD)/test_cor_smap.o: $(DEPS) \
	$(TEST)/test_cor_smap.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_smap.o $(TEST)/test_cor_smap.c

$(BUILD)/test_cor_spider.o: $(DEPS) \
	$(TEST)/test_cor_spider.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_spider.o $(TEST)/test_cor_spider.c

$(BUILD)/test_cor_thread.o: $(DEPS) \
	$(TEST)/test_cor_thread.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_thread.o $(TEST)/test_cor_thread.c

$(BUILD)/test_cor_trace.o: $(DEPS) \
	$(TEST)/test_cor_trace.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_trace.o $(TEST)/test_cor_trace.c

$(BUILD)/test_cor_uuid.o: $(DEPS) \
	$(TEST)/test_cor_uuid.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/test_cor_uuid.o $(TEST)/test_cor_uuid.c

clean:
	rm -rf $(BUILD)

prebuild:
	test -d $(BUILD) || mkdir -p $(BUILD)

install: all
	test -d /usr/local/include/core || mkdir -p /usr/local/include/core
	cp src/*.h /usr/local/include/core/
	cp objs/libcore.a /usr/local/lib/

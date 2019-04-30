INCLUDES = -I./
PREFIX = /usr/local
ifneq ("as","as${prefix}")
        PREFIX =${prefix}
endif
LIBDIR = $(PREFIX)/lib
##### Change the following for your environment:
COMPILE_OPTS =		$(INCLUDES) -I. -O2 -fPIC -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
C =			c
C_COMPILER =		cc
C_FLAGS =		$(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS)
CPP =			cpp
CPLUSPLUS_COMPILER =	c++
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS) -Wall -DBSD=1 $(CPPFLAGS) $(CXXFLAGS)
OBJ =			o
LINK =			c++ -o
LINK_OPTS =		-L. $(LDFLAGS)
CONSOLE_LINK_OPTS =	$(LINK_OPTS)
LIBRARY_LINK =		ar cr 
LIBRARY_LINK_OPTS =	
LIB_SUFFIX =			a
LIBS_FOR_CONSOLE_APPLICATION =
LIBS_FOR_GUI_APPLICATION =
EXE =
##### End of variables to change

NAME = libcommon
ALL = $(NAME).$(LIB_SUFFIX)
all:	$(ALL)

COMMON_FLAGS = -DENV_LINUX -D_GNU_SOURCE

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $(COMMON_FLAGS) $<
.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $(COMMON_FLAGS) $<

COMMON_LIB_OBJS = as_mutex.$(OBJ) as_thread.$(OBJ) as_event.$(OBJ) \
                  as_json.$(OBJ) as_queue.$(OBJ) as_time.$(OBJ) as_conn_manage.$(OBJ) \
                  as_daemon.$(OBJ) as_ini_config.$(OBJ) as_lock_guard.$(OBJ) \
                  as_log.$(OBJ) as_onlyone_process.$(OBJ) as_ring_cache.$(OBJ) \
                  as_timer.$(OBJ) as_tinyxml2.$(OBJ) as_http_digest.$(OBJ) as_base64.$(OBJ)

as_mutex.$(C):	as_mutex.h as_config.h as_common.h
as_thread.$(C):	as_thread.h as_config.h as_common.h
as_event.$(C):	as_event.h as_config.h as_common.h
as_json.$(C):	as_json.h as_config.h as_common.h
as_queue.$(C):	as_queue.h as_config.h as_common.h
as_time.$(C):	as_time.h as_config.h as_common.h
as_http_digest.$(C):	as_http_digest.h as_config.h as_common.h
as_base64.$(C):	as_base64.h as_config.h as_common.h
as_conn_manage.$(CPP):	as_conn_manage.h as_config.h as_common.h
as_daemon.$(CPP):	as_daemon.h as_config.h as_common.h
as_ini_config.$(CPP):	as_ini_config.h as_config.h as_common.h
as_lock_guard.$(CPP):	as_lock_guard.h as_config.h as_common.h
as_log.$(CPP):	as_log.h as_config.h as_common.h
as_onlyone_process.$(CPP):	as_onlyone_process.h as_config.h as_common.h
as_ring_cache.$(CPP):	as_ring_cache.h as_config.h as_common.h
as_timer.$(CPP):	as_timer.h as_config.h as_common.h
as_tinyxml2.$(CPP):	as_tinyxml2.h as_config.h as_common.h

$(NAME).$(LIB_SUFFIX): $(COMMON_LIB_OBJS) \
    $(PLATFORM_SPECIFIC_LIB_OBJS)
	$(LIBRARY_LINK)$@ $(LIBRARY_LINK_OPTS) \
		$(COMMON_LIB_OBJS)

clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~

install: install1 $(INSTALL2)
install1: $(NAME).$(LIB_SUFFIX)
	  install -d $(DESTDIR)$(PREFIX)/include/common $(DESTDIR)$(LIBDIR)
	  install -m 644 *.h $(DESTDIR)$(PREFIX)/include/common
	  install -m 644 $(NAME).$(LIB_SUFFIX) $(DESTDIR)$(LIBDIR)
install_shared_libraries: $(NAME).$(LIB_SUFFIX)
	  ln -fs $(NAME).$(LIB_SUFFIX) $(DESTDIR)$(LIBDIR)/$(NAME).$(SHORT_LIB_SUFFIX)
	  ln -fs $(NAME).$(LIB_SUFFIX) $(DESTDIR)$(LIBDIR)/$(NAME).so

##### Any additional, platform-specific rules come here:
